#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <stdbool.h>
#include "primitives.h"


char* read_attachment(char* file_path) {
    char * buffer = 0;
    long length;
    FILE * f = fopen(file_path, "rb");

    if (f)
    {
        fseek (f, 0, SEEK_END);
        length = ftell (f);
        fseek (f, 0, SEEK_SET);
        buffer = malloc (length*2);
        if (buffer)
        {
            fread (buffer, 1, length, f);
        }
        fclose (f);
    }

    if (buffer)
    {
        buffer[length] = '\0';
        return buffer;
    }
    return NULL;

}

char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* buffer needs to be freed */
char* base64_encode(char* buffer,size_t length) {
    /* 
     Made function super verbose with variables to avoid excessive comments, this is a simple base64 encoder
     It adds 1/3 to the input

     See this video for more details: https://www.youtube.com/watch?v=aUdKd0IFl34

    example:
        ASCII:                  A           B           C 
        Binary:             (01000001) (01000010) (01000011)
        B64Binary:          (010000) (010100) (001001) (000011)
        Base64 encoded:         Q        U      J         D
        
      at the end we pad with == or = depending on if there are one or two characters left over
     */

    char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    /* We turn every 3 bytes into 4 base64 chars, rounding up */
    size_t encoded_size = ((length + 2) / 3) * 4; 

    char* base64_content = (char*)malloc(encoded_size + 1);  /* don't get those off by one errors */
    if (!base64_content) {
        fprintf(stderr, "Failed to allocate memory for Base64 content\n");
        free(buffer);
        return NULL;
    }

    /* time to encode */
    size_t out_len = 0;
    for (int i = 0; i < (int)length; i += 3) {
        /* input 3 bytes from buffer, and output 4 base64 enc*/
        unsigned char input[3], output[4];
        int remaining = length - i;

        for (int j = 0; j < 3; j++) {
            /* copy 3 characters from the buffer into input, padding with 0s if required */
            input[j] = (i + j < (int)length) ? buffer[i + j] : 0;
        }

        unsigned char firstByte = input[0];
        unsigned char secondByte = input[1];
        unsigned char thirdByte = input[2];

        /* group 1 */
        unsigned char firstByteFirstSixBits = firstByte >> 2;
        unsigned char groupOne = firstByteFirstSixBits;

        /* group 2 */
        unsigned char firstByteLastTwoBits = firstByte & 0x03;
        unsigned char firstByteLastTwoBitsShiftedLeftToMakeRoomForTheNextFour = firstByteLastTwoBits << 4;
        unsigned char secondByteFirstFourBits = secondByte & 0xf0;
        unsigned char secondByteFirstFourBitsShiftedRightToMakeRoomForPreviousTwo = secondByteFirstFourBits >> 4;
        unsigned char groupTwo = firstByteLastTwoBitsShiftedLeftToMakeRoomForTheNextFour | secondByteFirstFourBitsShiftedRightToMakeRoomForPreviousTwo;  

        /* group 3 */
        unsigned char secondByteLastFourBits = secondByte & 0x0f;
        unsigned char secondByteLastFourBitsShiftedLeftToMakeRoomForNextTwo = secondByteLastFourBits << 2;
        unsigned char thirdBytefirstTwoBits = thirdByte & 0xc0;
        unsigned char thirdBytefirstTwoBitsShiftedRightToMakeRoomForPreviousFourBits = thirdBytefirstTwoBits >> 6;

        unsigned char groupThree = secondByteLastFourBitsShiftedLeftToMakeRoomForNextTwo | thirdBytefirstTwoBitsShiftedRightToMakeRoomForPreviousFourBits;

        /*group 4 */
        unsigned char thirdByteLastSixBits = thirdByte & 0x3f;
        unsigned char groupFour = thirdByteLastSixBits;



        output[0] = base64_table[groupOne];

        output[1] = base64_table[groupTwo];

        output[2] = (remaining > 1) ? base64_table[groupThree] : '=';

        output[3] = (remaining > 2) ? base64_table[groupFour] : '=';

        for (int j = 0; j < 4; j++) {
            base64_content[out_len++] = output[j];
        }
    }

    base64_content[out_len] = '\0'; 


    return base64_content;
}


char* read_attachment_b64(const char* file_path) {
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", file_path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size+1);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for file content\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, file_size, file);
    fclose(file);

    char* base64_content = base64_encode(buffer,file_size);
    // Calculate the size of the Base64-encoded buffer

    return base64_content;
}

const char* get_content_type(fileType file_extension) {
    switch (file_extension) {
        case TXT: return "text/plain";
        case CSV: return "text/plain";
        case JPG: return "image/jpeg";
        case PNG: return "image/png";
        case PDF: return "application/pdf";
        case UNKNOWN: return "octet-stream";
    }
    return "octet-stream";
}


char* compose_email(Email email,int force) {

    char* boundary_text = "XXXXboundary text";
    bool sendAttachments = false;
    char attachment_content[ATTACHMENT_SIZE*MAX_ADDR_AMOUNT] = { 0 };

    /* loop over attachs if they exist and make text to add to email payload */
    if (email.amount_of_attachments > 0) {
        int attachmentsParsed = 0;
        for (int i = 0; i<email.amount_of_attachments;i++) {
            char tmp[ATTACHMENT_SIZE-1] = { 0 };
            char* attachment_buffer = read_attachment_b64(email.attachments[i].filepath);
            if (attachment_buffer != NULL) {
                if (strlen(attachment_buffer) > 0) {
                    attachmentsParsed++;
                }
                else {
                    fprintf(stderr,"failed to load content from: %s\n",email.attachments[i].filepath);
                }


                char* encoding = "Content-Transfer-Encoding: base64\r\n";

                /* add to attachment content ready to be appended to email payload */
                snprintf(tmp,ATTACHMENT_SIZE-1,
                        "\r\n--%s\r\n"
                        "Content-Type: %s;\r\n"
                        "Content-Disposition: attachment;\r\n"
                        "\tfilename=\"%s\"\r\n"
                         "%s\r\n"
                        "\r\n"
                        "%s\r\n"
                        "\r\n"
                        "\r\n--%s\r\n",
                        boundary_text,get_content_type(email.attachments[i].filetype),email.attachments[i].name,encoding,attachment_buffer,boundary_text);

                free(attachment_buffer);

                strncat(attachment_content, tmp, ATTACHMENT_SIZE-1);

            }
            else if (force) {
                printf("[-] failed to load content from: %s, sending anyway due to --force\n",email.attachments[i].filepath);
            }
            else {
                fprintf(stderr,"\nCould not read attachment %s\n",email.attachments[i].filepath);
                exit(EXT_ATTACHMENT_ERR);
            }
        }
        /* check parsing success */
        if (email.amount_of_attachments == attachmentsParsed) {  
            printf("[+] Retrieved all %d attachments content",email.amount_of_attachments);
            sendAttachments = true;
        }
        else {
            printf("[-] failed to load %d/%d attachments content",email.amount_of_attachments-attachmentsParsed,email.amount_of_attachments);
            if (force && (attachmentsParsed > 0)) {
                sendAttachments = true;
                printf("sending anyway due to --force");
            }
            printf("\n");
        }
    }

    /* compose addresses as csv strings */
    char addressesLine[TO_SIZE] = { 0 };
    char ccaddressesLine[CC_SIZE] = { 0 };
    char bccaddressesLine[BCC_SIZE] = { 0 };

    for (int a = 0;a < email.amount_of_addresses; a++) {
        char tmp[TO_SIZE] = { 0 };
        if (strlen(email.addresses[a].name) > 0) {
            sprintf(tmp, " %s <%s>%s",email.addresses[a].name,email.addresses[a].address,a == (email.amount_of_addresses - 1) ? "" : ",");
        }
        else {
            sprintf(tmp, " %s%s",email.addresses[a].address,a == (email.amount_of_addresses - 1) ? "" : ",");
        }
        strcat(addressesLine, tmp);
    }
    for (int a = 0;a < email.amount_of_ccaddresses; a++) {
        char tmp[CC_SIZE] = { 0 };
        if (strlen(email.ccaddresses[a].name) > 0) {
            sprintf(tmp, " %s <%s>%s",email.ccaddresses[a].name,email.ccaddresses[a].address,a == (email.amount_of_ccaddresses - 1) ? "" : ",");
        }
        else {
            sprintf(tmp, " %s%s",email.ccaddresses[a].address,a == (email.amount_of_ccaddresses - 1) ? "" : ",");
        }
        strcat(ccaddressesLine, tmp);
    }
    for (int a = 0;a < email.amount_of_bccaddresses; a++) {
        char tmp[BCC_SIZE] = { 0 };
        if (strlen(email.bccaddresses[a].name) > 0) {
            sprintf(tmp, " %s <%s>%s",email.bccaddresses[a].name,email.bccaddresses[a].address,a == (email.amount_of_bccaddresses - 1) ? "" : ",");
        }
        else {
            sprintf(tmp, " %s%s",email.bccaddresses[a].address,a == (email.amount_of_bccaddresses - 1) ? "" : ",");
        }
        strcat(bccaddressesLine, tmp);
    }
    char fromLine[FROM_SIZE] = { 0 };
    sprintf(fromLine,"%s <%s>",email.fromUser,email.fromUsername);
    /* composed addresses as csv strings and set from */


    char* payload_text = malloc(sizeof(char)*PAYLOAD_SIZE);
    memset(payload_text,'\0',sizeof(char)*PAYLOAD_SIZE);

    snprintf(payload_text, PAYLOAD_SIZE,
            "To: %s \r\n"
            "From: %s \r\n"
            "Cc: %s \r\n"
            "MIME-Version: 1.0\r\n"
            "Content-Type: multipart/mixed;\r\n"
            "\tboundary=\"%s\"\r\n"
            "Subject: %s \r\n\r\n"
            "\r\n"
            "--%s\r\n"
            "Content-Type: text/plain;\r\n"
            "\r\n"
            "%s\r\n",
            addressesLine, fromLine, ccaddressesLine, boundary_text, email.subject,boundary_text,email.body);


    if (email.amount_of_attachments > 0 && sendAttachments) {
        strcat(payload_text,attachment_content);
    }
    else {
        char tmp[100] = { 0 };
        sprintf(tmp, 
            "--%s\r\n"
             "\r\n",
                boundary_text);
        strcat(payload_text,tmp);
    }

    printf("%s\n",payload_text);

    return payload_text;
}


