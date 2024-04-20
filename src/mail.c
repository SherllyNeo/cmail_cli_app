#include <curl/curl.h>
#include "parser.h"
#include "primitives.h"


char payload_text[PAYLOAD_SIZE];

struct upload_status {
    size_t bytes_read;
};

static size_t payload_source(char *ptr,size_t size,size_t nmemb,void *userp) {

    struct upload_status *upload_ctx = (struct upload_status *)userp;
    const char *data;
    size_t room = size * nmemb;

    if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
        return 0;
    }

    data = &payload_text[upload_ctx->bytes_read];

    if(data) {
        size_t len = strlen(data);
        if(room < len)
            len = room;
        memcpy(ptr, data, len);
        upload_ctx->bytes_read += len;

        return len;
    }

    return 0;
}


void send_email(Email email,int force,char* user,char* username, char* smtp,char* pass) {
    
    CURL *curl;
    CURLcode res_ = CURLE_OK;
    struct curl_slist *recipients = NULL;
    struct upload_status upload_ctx = { 0 };

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl,CURLOPT_USE_SSL,(long)CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_USERNAME, user);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, pass);

        curl_easy_setopt(curl, CURLOPT_URL, smtp);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, user);
        curl_easy_setopt(curl, CURLOPT_MAIL_AUTH, user);

        if (email.amount_of_addresses > 0) {
            for (int i =0; i<email.amount_of_addresses;i++) {
                recipients = curl_slist_append(recipients, email.addresses[i].address);
            }
            for (int i =0; i<email.amount_of_ccaddresses;i++) {
                recipients = curl_slist_append(recipients, email.ccaddresses[i].address);
            }

            char* email_txt = compose_email(email, force);
            strcpy(payload_text,email_txt);
            free(email_txt);

            printf("%s\n",payload_text);
            curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
            curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

            /* Send the message */
            res_ = curl_easy_perform(curl);

            /* Check for errors */
            if(res_ != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() failed: %s\n",curl_easy_strerror(res_));
            }
            else {
                time_t t = time(NULL);
                struct tm *tm = localtime(&t);
                printf("\n[+] sent emails at: %s\n",asctime(tm));
            }

            /* Free the list of recipients */
            curl_slist_free_all(recipients);
        }


        /* send to blind carbon copy emails */
        if (email.amount_of_bccaddresses > 0) {
            for (int i =0; i<email.amount_of_bccaddresses;i++) {
                Address nextAddr[MAX_ADDR_LENGTH];
                memset(nextAddr,0,sizeof(Address)*MAX_ADDR_LENGTH);
                nextAddr[0] = email.bccaddresses[i];

                Email newEmail = initEmail(user, username, nextAddr,1, NULL,0,NULL, 0,email.attachments, email.amount_of_attachments, email.subject, email.body);
                send_email(newEmail, force, user, username, smtp, pass);
            }
        }
        
        if (email.amount_of_addresses <= 0 && email.amount_of_bccaddresses <= 0) {
            fprintf(stderr,"ERROR: No addresses to send, fatal\n");
            curl_easy_cleanup(curl);
            exit(EXT_ARG_PARSING);
        }
        curl_easy_cleanup(curl);

    }
}






