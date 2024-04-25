#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <stdbool.h>
#include "shared.h"


char* readAttachment(char* filepath,size_t* file_size) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(*file_size+1);

    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for file content\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, *file_size, file);
    fclose(file);

    return buffer;

}


char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* buffer needs to be freed */
char* base64EncodeFunc(char* buffer,size_t length,size_t* encodedlen) {
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

    *encodedlen = out_len;

    return base64_content;
}


char* readAttachmentEncoded(char* filepath,size_t* file_size,char*(*encoder)(char*,size_t,size_t*)) {

    char* content = readAttachment(filepath,file_size);

    char* encoded_content = encoder(content,*file_size,file_size);

    return encoded_content;
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

/* PUBLIC FUNCTIONS - preappended with composer to show which file it came from */

char* composerComposeEmail(Email email,int force) {
    char* boundary_text = "XXXXboundary text";
    bool send_attachments = false;
    //char attachment_content[TOTAL_MAX_ATTACH_SIZE] = { 0 };
    size_t attachment_content_size = TOTAL_MAX_ATTACH_SIZE;
    char* attachment_content = malloc(attachment_content_size*sizeof(char));
    if (attachment_content == NULL) {
        fprintf(stderr,"Unable to allocate attachment content\n");
        exit(EXT_ATTACHMENT_ERR);
    }
    size_t total_attachsize_so_far = 0;

    /* loop over attachs if they exist and make text to add to email payload */
    if (email.amount_of_attachments > 0) {
        int attachmentsParsed = 0;
        for (int i = 0; i<email.amount_of_attachments;i++) {
            /* attachment_size will fill up with the size of the content */
            size_t attachment_buffer_size = 0;
            size_t attachment_size = 0;
            char* attachment_buffer = readAttachmentEncoded(email.attachments[i].filepath,&attachment_buffer_size,&base64EncodeFunc);


            char* encoding = "Content-Transfer-Encoding: base64\r\n";
            attachment_size = snprintf((char *)NULL,0,
                    "\r\n--%s\r\n"
                    "Content-Type: %s;\r\n"
                    "Content-Disposition: attachment;\r\n"
                    "\tfilename=\"%s\"\r\n"
                    "%s\r\n"
                    "\r\n"
                    "%s\r\n"
                    "\r\n",
                    boundary_text,get_content_type(email.attachments[i].filetype),email.attachments[i].name,encoding,attachment_buffer);

            char* tmp = malloc((attachment_size+1)*sizeof(char));
            memset(tmp,'\0',(attachment_size+1)*sizeof(char));

            /* add to attachment content ready to be appended to email payload */
            snprintf(tmp,attachment_size,
                    "\r\n--%s\r\n"
                    "Content-Type: %s;\r\n"
                    "Content-Disposition: attachment;\r\n"
                    "\tfilename=\"%s\"\r\n"
                    "%s\r\n"
                    "\r\n"
                    "%s\r\n"
                    "\r\n",
                    boundary_text,get_content_type(email.attachments[i].filetype),email.attachments[i].name,encoding,attachment_buffer);


            bool attach = true;
            if ((attachment_content_size - total_attachsize_so_far) <= attachment_size ) {
                fprintf(stderr,"[-] unable to load all content from %s as the attachment size limit has been reached (%zu bytes). \n\
                        There was %zu bytes remaining, and we are trying to attach %zu bytes\n"
                        ,email.attachments[i].filepath,attachment_content_size,(attachment_content_size-total_attachsize_so_far), attachment_size);
                        
                        fprintf(stderr,"[-] Reallocing to try and fix...\n");
                        attachment_content_size *= 2;
                        char* tmp_ptr = (char*)realloc(attachment_content,attachment_content_size*sizeof(char));
                        if (tmp_ptr == NULL) {
                            fprintf(stderr,"[-] Unable to allocate attachment content\n");
                            attach = false;
                        }
                        else {
                            attachment_content = tmp_ptr;
                        }
            }

            if (attachment_buffer == NULL || strlen(attachment_buffer) <= 0 || attachment_size <= 0) {
                    fprintf(stderr,"[-] unable to load any content from %s\n",email.attachments[i].filepath);
                    attach = false;
            }
            free(attachment_buffer);

            if (attach) {
                strncat(attachment_content, tmp,attachment_content_size - total_attachsize_so_far);
                total_attachsize_so_far += attachment_size;
                attachmentsParsed++;
            }
            else {
                fprintf(stderr,"[-] will not attach %s\n",email.attachments[i].filepath);
            }
            free(tmp);
        }
        /* check parsing success */
        if (email.amount_of_attachments == attachmentsParsed) {  
            printf("[+] Retrieved all %d attachments content\n",email.amount_of_attachments);
            send_attachments = true;
        }
        else {
            printf("[-] failed to load %d/%d attachments content, ",email.amount_of_attachments-attachmentsParsed,email.amount_of_attachments);
            if (force && (attachmentsParsed > 0)) {
                send_attachments = true;
                printf("sending anyway due to --force");
            }
            else {
                printf("--force is not set, so crashing gracefully\n");
                exit(EXT_ATTACHMENT_ERR);
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
            "%s\r\n"
            "\r\n",
            addressesLine, fromLine, ccaddressesLine, boundary_text, email.subject,boundary_text,email.body);


    if (email.amount_of_attachments > 0 && send_attachments) {
        strcat(payload_text,attachment_content);
    }
    /* add the bottom bounary */
   // char tmp[100] = { 0 };
   // sprintf(tmp, 
   //         "--%s\r\n",
   //         boundary_text);
   // strcat(payload_text,tmp);

    printf("%s\n",payload_text);
    return payload_text;
}


