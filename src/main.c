#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include "parser.h"
#include "primitives.h"
#include "mail.h"

const char banner[] = "\n \
 ____ ___ __  __ ____  _     _______  __  __  __    _    ___ _ \n\
/ ___|_ _|  \\/  |  _ \\| |   | ____\\ \\/ / |  \\/  |  / \\  |_ _| | \n\
\\___ \\| || |\\/| | |_) | |   |  _|  \\  /  | |\\/| | / _ \\  | || | \n\
 ___) | || |  | |  __/| |___| |___ /  \\  | |  | |/ ___ \\ | || |___ \n\
|____/___|_|  |_|_|   |_____|_____/_/\\_\\ |_|  |_/_/   \\_\\___|_____| \n\
";


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
                     You can write \"name@example.com:FirstName MiddleName Lastname\" for the email to be sent To: FirstName MiddleName LastName <name@example.com>\n \
                     You can write /path/to/attachmentPath.txt:NewAttachmentName.txt for the email to show AttachmentName.txt\n \
                     \n \
                     Useage: mailer -s \"subject\" -b \"body\" -t \"user@example.com:user name, user1@example.com: firstname lastname\" -c \"user2@example.com\" -a \"file.pdf\" \n \
                     This will email as To: user name <user@example.com>, firstname lastname <user1@example.com> and cc in user2@example.com. The email will have subject, \"subject\" and body \"body\" and send the pdf file \n \
                     ";

int main(int argc, char* argv[]) {
    printf("\n%s\n\n",banner);


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

    if ((addrsAmount+ccaddrsAmount+bccaddrsAmount) > MAX_ADDR_AMOUNT) {
        fprintf(stderr,"\nERROR: Too many to email addresses\n");
        exit(EXT_ARG_PARSING);
    }
    if (attachmentsAmount > MAX_ATTACH_AMOUNT) {
        fprintf(stderr,"\nERROR: Too many to attachments\n");
        exit(EXT_ARG_PARSING);
    }

    printf("[+] Parsed addresses\n");
    Email email = initEmail(EMAIL_USERNAME,EMAIL_USER,addresses,addrsAmount,ccaddresses,ccaddrsAmount,bccaddresses,bccaddrsAmount,attachments,attachmentsAmount,subject,body);
    printf("[+] Created email\n");
    send_email(email,force, EMAIL_USER,EMAIL_USERNAME, EMAIL_SMTP, EMAIL_PASS);
    printf("\n[+] Done :)\n");

    return EXIT_SUCCESS;

}


