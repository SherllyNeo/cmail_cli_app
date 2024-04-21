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

char* read_attachment_b64(const char* file_path) {
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", file_path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for file content\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, file_size, file);
    fclose(file);

    // Calculate the size of the Base64-encoded buffer
    size_t encoded_size = ((file_size + 2) / 3) * 4; // 4/3 * input_size rounded up

    // Allocate memory for the Base64-encoded string
    char* base64_content = (char*)malloc(encoded_size + 1); // +1 for null terminator
    if (!base64_content) {
        fprintf(stderr, "Failed to allocate memory for Base64 content\n");
        free(buffer);
        return NULL;
    }

    // Base64 encode the buffer
    size_t out_len = 0;
    for (int i = 0; i < file_size; i += 3) {
        unsigned char input[3], output[4];
        int remaining = file_size - i;

        for (int j = 0; j < 3; j++) {
            input[j] = (i + j < file_size) ? buffer[i + j] : 0;
        }

        output[0] = base64_table[input[0] >> 2];
        output[1] = base64_table[((input[0] & 0x03) << 4) | ((input[1] & 0xf0) >> 4)];
        output[2] = (remaining > 1) ? base64_table[((input[1] & 0x0f) << 2) | ((input[2] & 0xc0) >> 6)] : '=';
        output[3] = (remaining > 2) ? base64_table[input[2] & 0x3f] : '=';

        for (int j = 0; j < 4; j++) {
            base64_content[out_len++] = output[j];
        }
    }

    base64_content[out_len] = '\0'; // Null-terminate the string

    free(buffer);
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
    char attachment_content[ATTACHMENT_SIZE];

    /* loop over attachs if they exist and make text to add to email payload */
    if (email.amount_of_attachments > 0) {
        int attachmentsParsed = 0;
        for (int i = 0; i<email.amount_of_attachments;i++) {
            char tmp[ATTACHMENT_SIZE/email.amount_of_attachments];
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
                snprintf(tmp,ATTACHMENT_SIZE/email.amount_of_attachments,
                        "Content-Type: %s;\r\n"
                        "Content-Disposition: attachment;\r\n"
                        "\tfilename=\"%s\"\r\n"
                         "%s\r\n"
                        "\r\n"
                        "%s\r\n"
                        "--%s\r\n",
                        get_content_type(email.attachments[i].filetype),email.attachments[i].name,encoding,attachment_buffer,boundary_text);

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
    char addressesLine[TO_SIZE];
    char ccaddressesLine[CC_SIZE];
    char bccaddressesLine[BCC_SIZE];

    for (int a = 0;a < email.amount_of_addresses; a++) {
        char tmp[TO_SIZE/email.amount_of_addresses];
        if (strlen(email.addresses[a].name) > 0) {
            sprintf(tmp, " %s <%s>%s",email.addresses[a].name,email.addresses[a].address,a == (email.amount_of_addresses - 1) ? "" : ",");
        }
        else {
            sprintf(tmp, " %s%s",email.addresses[a].address,a == (email.amount_of_addresses - 1) ? "" : ",");
        }
        strcat(addressesLine, tmp);
    }
    for (int a = 0;a < email.amount_of_ccaddresses; a++) {
        char tmp[TO_SIZE/email.amount_of_ccaddresses];
        if (strlen(email.ccaddresses[a].name) > 0) {
            sprintf(tmp, " %s <%s>%s",email.ccaddresses[a].name,email.ccaddresses[a].address,a == (email.amount_of_ccaddresses - 1) ? "" : ",");
        }
        else {
            sprintf(tmp, " %s%s",email.ccaddresses[a].address,a == (email.amount_of_ccaddresses - 1) ? "" : ",");
        }
        strcat(ccaddressesLine, tmp);
    }
    for (int a = 0;a < email.amount_of_bccaddresses; a++) {
        char tmp[TO_SIZE/email.amount_of_bccaddresses];
        if (strlen(email.bccaddresses[a].name) > 0) {
            sprintf(tmp, " %s <%s>%s",email.bccaddresses[a].name,email.bccaddresses[a].address,a == (email.amount_of_bccaddresses - 1) ? "" : ",");
        }
        else {
            sprintf(tmp, " %s%s",email.bccaddresses[a].address,a == (email.amount_of_bccaddresses - 1) ? "" : ",");
        }
        strcat(bccaddressesLine, tmp);
    }
    char fromLine[FROM_SIZE];
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
            "\r\n--%s"
             "\r\n",
            addressesLine, fromLine, ccaddressesLine, boundary_text, email.subject,boundary_text,email.body,boundary_text);


    if (email.amount_of_attachments > 0 && sendAttachments) {
        strcat(payload_text,attachment_content);
    }

    return payload_text;
}


