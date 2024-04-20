#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include "parser.h"
#include "composer.h"
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






const char help[] = "\nsimplex mailer \n---------------------- \n\n \
                     -h or --help to view help\n \
                     -t or --toAddrs sets the to addresses\n \
                     -cc or --ccAddrs sets CC addresses\n \
                     -bc or --bccAddrs sets BCC addresses\n \
                     -s or --subject sets subject\n \
                     -b or --body sets body\n \
                     -a or --attachment_path\n \
                     Attachment name will default to attachment path name if not specified\n \
                     -f  or --force alone will ensure email is sent without an attachment if an attachment isn't found. rather than exiting\n \
                     \n \
                     You can write name@example.com:FirstName-MiddleName-Lastname for the email to be sent To: FirstName MiddleName LastName <name@example.com>\n \
                     You can write attachmentPath.txt:NewAttachmentName.txt for the email to show AttachmentName.txt\n \
                     ";

int main(int argc, char* argv[]) {
    printf("\nSimplex Mailer\n\n");


    /* check envrioment is correct */
    char* EMAIL_PASS = getenv("EMAIL_PASS");
    char* EMAIL_USERNAME = getenv("EMAIL_USERNAME");
    char* EMAIL_USER = getenv("EMAIL_USER");
    char* EMAIL_SMTP = getenv("EMAIL_SMTP");
    if (EMAIL_PASS == NULL) {
        fprintf(stderr, "unable to get EMAIL_PASS, please set in envrioment");
        exit(ERR_ENV_PARSING);
    }
    if (EMAIL_USERNAME == NULL) {
        fprintf(stderr, "unable to get EMAIL_USERNAME, please set in envrioment");
        exit(ERR_ENV_PARSING);
    }

    if (EMAIL_USERNAME == NULL) {
        fprintf(stderr, "unable to get EMAIL_USER, please set in envrioment");
        exit(ERR_ENV_PARSING);
    }

    if (EMAIL_SMTP == NULL) {
        fprintf(stderr, "unable to get EMAIL_SMTP, please set in envrioment with format smtp://smtp.example.com:<PORT NUMBER>");
        exit(ERR_ENV_PARSING);
    }

    if (EMAIL_USER && EMAIL_USERNAME && EMAIL_PASS && EMAIL_SMTP) {
        printf("[+] Retrieved enviroment variables\n");
    }
    char* toAddrs = NULL;
    char* ccAddrs = NULL;
    char* bccAddrs = NULL;
    char* subject = NULL;
    char* body = NULL;
    char* attachments_ = NULL;
    int force = 0;

    for (int i=1;i<argc;i++) {
        char* current_arg = argv[i];
        if (!strcmp("-h",current_arg) || !strcmp("--help",current_arg)) {
            printf("\n%s\n",help);
            return 0;
        }
        else if (!strcmp("-t",current_arg) || !strcmp("--toAddrs",current_arg)) {
            toAddrs = argv[++i];
        }
        else if (!strcmp("-cc",current_arg) || !strcmp("--ccAddrs",current_arg)) {
            ccAddrs = argv[++i];
        }
        else if (!strcmp("-bc",current_arg) || !strcmp("--bccAddrs",current_arg)) {
            bccAddrs = argv[++i];
        }
        else if (!strcmp("-s",current_arg) || !strcmp("--subject",current_arg)) {
            subject = argv[++i];
        }
        else if (!strcmp("-b",current_arg) || !strcmp("--body",current_arg)) {
            body = argv[++i];
        }
        else if (!strcmp("-a",current_arg) || !strcmp("--attachments",current_arg)) {
            attachments_ = argv[++i];
        }
        else if (!strcmp("-f",current_arg) || !strcmp("--force",current_arg)) {
            force = 1;
        }
    }

    /* check requirements */
    if (!toAddrs && !bccAddrs) {
        printf("\nNo to addresses found \n%s\n",help);
        fprintf(stderr,"\nERROR: Too few arguments. Need at least sending address or blind carbon copy address\n");
        exit(EXT_ARG_PARSING);
    }
    if (!subject) {
        subject = "";
    }
    if (!body) {
        body = "";
    }

    /* read in addresses, attachments and more */
    Address addresses[MAX_ADDR_AMOUNT];
    Address ccaddresses[MAX_ADDR_AMOUNT];
    Address bccaddresses[MAX_ADDR_AMOUNT];
    Attachment attachments[MAX_ATTACH_AMOUNT];
    int addrsAmount = 0;
    int ccaddrsAmount = 0;
    int bccaddrsAmount = 0;
    int attachmentsAmount = 0;

    if (toAddrs) {
        addrsAmount = parseAddresses(toAddrs, addresses,"TO");
    }

    if (ccAddrs) {
        ccaddrsAmount = parseAddresses(ccAddrs, ccaddresses,"CC");
    }

    if (bccAddrs) {
        bccaddrsAmount = parseAddresses(bccAddrs, bccaddresses,"BCC");
    }

    if (attachments_) {
        attachmentsAmount = parseAttachments(attachments_, attachments);
    }

    if (addrsAmount <= 0 ) {
        if (bccAddrs && bccaddrsAmount >= 0) {
            printf("INFO: No to addresses, but sending to all blind carbon copies\n");
        }
        else {
            fprintf(stderr,"\nERROR: No valid sending addresses\n");
            exit(EXT_ARG_PARSING);
        }
    }

    if (attachmentsAmount <= 0 && attachments_ && !force) {
        fprintf(stderr,"\nERROR: No valid attachments and no permission to send without them using -f\n");
        exit(EXT_ARG_PARSING);
    }

    printf("[+] Parsed addresses\n");
    Email email = initEmail(EMAIL_USERNAME,EMAIL_USER,addresses,addrsAmount,ccaddresses,ccaddrsAmount,bccaddresses,bccaddrsAmount,attachments,attachmentsAmount,subject,body);
    printf("[+] Parsed email\n");
    send_email(email,force, EMAIL_USER,EMAIL_USERNAME, EMAIL_SMTP, EMAIL_PASS);





    return EXIT_SUCCESS;

}


