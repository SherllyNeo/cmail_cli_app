#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include "parser.h"
#include "shared.h"
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
                     -t or --to_addresses sets the to addresses\n \
                     -c or --carbon_copy_addresses sets CC addresses\n \
                     -bc or --blind_carbon_copy_addresses BCC addresses\n \
                     -s or --subject sets subject\n \
                     -b or --body sets body\n \
                     -a or --attachment_paths\n \
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

    char* to_addresses_arg = NULL;
    char* cc_addresses_arg = NULL;
    char* bcc_addresses_arg = NULL;
    char* subject = NULL;
    char* body = NULL;
    char* attachments_arg = NULL;
    int force = 0;

    for (int i=1;i<argc;i++) {
        char* current_arg = argv[i];
        if (!strcmp("-h",current_arg) || !strcmp("--help",current_arg)) {
            printf("\n%s\n",help);
            return 0;
        }
        else if (!strcmp("-t",current_arg) || !strcmp("--to_addresses",current_arg)) {
            to_addresses_arg = argv[++i];
        }
        else if (!strcmp("-cc",current_arg) || !strcmp("--carbon_copy_addresses",current_arg)) {
            cc_addresses_arg = argv[++i];
        }
        else if (!strcmp("-bc",current_arg) || !strcmp("--blind_carbon_copy_addresses",current_arg)) {
            bcc_addresses_arg = argv[++i];
        }
        else if (!strcmp("-s",current_arg) || !strcmp("--subject",current_arg)) {
            subject = argv[++i];
        }
        else if (!strcmp("-b",current_arg) || !strcmp("--body",current_arg)) {
            body = argv[++i];
        }
        else if (!strcmp("-a",current_arg) || !strcmp("--attachment_paths",current_arg)) {
            attachments_arg = argv[++i];
        }
        else if (!strcmp("-f",current_arg) || !strcmp("--force",current_arg)) {
            force = 1;
        }
    }

    /* check requirements */
    if (!to_addresses_arg && !bcc_addresses_arg) {
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
    int addresses_amount = 0;
    int cc_addresses_amount = 0;
    int bcc_addresses_amount = 0;
    int attachment_amount = 0;

    if (to_addresses_arg) {
        addresses_amount = parserParseAddresses(to_addresses_arg, addresses,"TO");
    }

    if (cc_addresses_arg) {
        cc_addresses_amount = parserParseAddresses(cc_addresses_arg, ccaddresses,"CC");
    }

    if (bcc_addresses_arg) {
        bcc_addresses_amount = parserParseAddresses(bcc_addresses_arg, bccaddresses,"BCC");
    }


    if (attachments_arg) {
        attachment_amount = parserParseAttachments(attachments_arg, attachments);
    }

    if (addresses_amount <= 0 ) {
        if (bcc_addresses_arg && bcc_addresses_amount >= 0) {
            printf("INFO: No to addresses, but sending to all blind carbon copies\n");
        }
        else {
            fprintf(stderr,"\nERROR: No valid sending addresses\n");
            exit(EXT_ARG_PARSING);
        }
    }

    if (attachment_amount <= 0 && attachments_arg && !force) {
        fprintf(stderr,"\nERROR: No valid attachments and no permission to send without them using -f\n");
        exit(EXT_ARG_PARSING);
    }

    if ((addresses_amount+cc_addresses_amount+bcc_addresses_amount) > MAX_ADDR_AMOUNT) {
        fprintf(stderr,"\nERROR: Too many to email addresses\n");
        exit(EXT_ARG_PARSING);
    }
    if (attachment_amount > MAX_ATTACH_AMOUNT) {
        fprintf(stderr,"\nERROR: Too many to attachments\n");
        exit(EXT_ARG_PARSING);
    }

    printf("[+] Parsed addresses\n");
    Email email = parserInitEmail(EMAIL_USERNAME,EMAIL_USER,addresses,addresses_amount,ccaddresses,cc_addresses_amount,bccaddresses,bcc_addresses_amount,attachments,attachment_amount,subject,body);
    printf("[+] Created email\n");
    mailSendEmail(email,force, EMAIL_USER,EMAIL_USERNAME, EMAIL_SMTP, EMAIL_PASS);
    printf("\n\n[+] Done :)\n");

    return EXIT_SUCCESS;

}


