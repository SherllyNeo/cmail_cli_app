#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include "primitives.h"

#define SUBJECT_SIZE 500
#define BODY_SIZE 5000
#define TO_SIZE 200
#define FROM_SIZE 500
#define CC_SIZE 200
#define BCC_SIZE 200
#define ATTACHMENT_SIZE 90000
#define PAYLOAD_SIZE (SUBJECT_SIZE + BODY_SIZE + ATTACHMENT_SIZE)*3

#define EXT_ATTACHMENT_ERR 5
#define EXT_EMAIL_ERR 6
#define EXT_CONNECTION_ERR 7


char *base64Encode(const unsigned char *data, size_t input_length, size_t *output_length) {
    static const char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t output_buffer_size = 4 * ((input_length + 2) / 3);
    char *output = (char *)malloc(output_buffer_size);
    if (!output) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        output[j++] = base64_table[(triple >> 3 * 6) & 0x3F];
        output[j++] = base64_table[(triple >> 2 * 6) & 0x3F];
        output[j++] = base64_table[(triple >> 1 * 6) & 0x3F];
        output[j++] = base64_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < 3 - (int)input_length % 3; i++) {
        output[output_buffer_size - 1 - i] = '=';
    }

    *output_length = output_buffer_size;
    return output;
}

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

int count_lines_of_file(char* file_path) {
    FILE * fp;
    int lines = 0;
    char ch;
    if ((fp = fopen(file_path, "r")) == NULL) {
        fprintf(stderr,"Error! opening file\n");
        exit(0);
    }

    while(!feof(fp))
    {
        ch = fgetc(fp);
        if(ch == '\n')
        {
            lines++;
        }
    }

    return lines;

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
            char* attachment_buffer = read_attachment(email.attachments[i].filepath);
            if (attachment_buffer != NULL) {
                if (strlen(attachment_buffer) > 0) {
                    attachmentsParsed++;
                }
                else {
                    fprintf(stderr,"failed to load content from: %s\n",email.attachments[i].filepath);
                }

                char* encoding = "";
                if (email.attachments[i].filetype == PDF || email.attachments[i].filetype == JPG || email.attachments[i].filetype == PNG) {
                    encoding = "Content-Transfer-Encoding: base64\r\n";
                    // Base64 encode PDF content
                    size_t pdfContentSize = strlen(attachment_buffer);
                    size_t encodedLength;
                    char *encodedPdfContent = base64Encode((const unsigned char *)attachment_buffer, pdfContentSize, &encodedLength);
                    if (!encodedPdfContent) {
                        fprintf(stderr, "[-] Failed to encode PDF content. Using raw\n");
                        free(encodedPdfContent);
                    }
                    else {
                        strcpy(attachment_buffer,encodedPdfContent);
                    }
                }

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
            "\r\n--%s",
            addressesLine, fromLine, ccaddressesLine, boundary_text, email.subject,boundary_text,email.body,boundary_text);


    if (email.amount_of_attachments > 0 && sendAttachments) {
        strcat(payload_text,attachment_content);
    }

    return payload_text;
}


