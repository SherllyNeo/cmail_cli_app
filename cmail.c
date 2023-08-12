#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "emailer.h"
#include "shared.h"



char help[] = "\nSherlly's simple cmail \n---------------------- \n\n \
-h or --help to view help\n \
-t or --to_addr sets to address\n \
-n or --to_name sets to name\n \
-c or --cc_addr sets cc address\n \
-s or --subject sets subject\n \
-b or --body sets body\n \
if you want attachment, both of these must be specified\n \
-a or --attachment_path sets attachment_path\n \
-an or --attachment_name sets attachment_path\n";

int main(int argc, char* argv[]) {
    char* to_addr;
    char* to_name;
    char* cc_addr = "";
    char* subject;
    char* body;
    char* attachment_path = NULL;
    char* attachment_name = NULL;

	if (argc<5 && argc!=2) {
	printf("\nYou need at least 5 arguments. To Address, To name, CC address, Subject, Body and optional attachment path and name\n");
	fprintf(stderr,"\nToo few arguments, needs at least five \n");
	exit(0);
	}
    for (int i=1;i<argc;i++) {
        char* current_arg = argv[i];
        if (!strcmp("-h",current_arg) || !strcmp("--help",current_arg)) {
            printf("\n%s\n",help);
            return 0;
        }
        else if (!strcmp("-t",current_arg) || !strcmp("--to_addr",current_arg)) {
            to_addr = argv[++i];
        }
        else if (!strcmp("-n",current_arg) || !strcmp("--to_name",current_arg)) {
            to_name = argv[++i];
        }
        else if (!strcmp("-c",current_arg) || !strcmp("--cc_addr",current_arg)) {
            cc_addr = argv[++i];
        }
        else if (!strcmp("-s",current_arg) || !strcmp("--subject",current_arg)) {
            subject = argv[++i];
        }
        else if (!strcmp("-b",current_arg) || !strcmp("--body",current_arg)) {
            body = argv[++i];
        }
        else if (!strcmp("-a",current_arg) || !strcmp("--attachment_path",current_arg)) {
            attachment_path = argv[++i];
        }
        else if (!strcmp("-an",current_arg) || !strcmp("--attachment_name",current_arg)) {
            attachment_name = argv[++i];
        }
    }
	struct Email email = {to_addr,to_name,cc_addr,subject,body,attachment_path,attachment_name};

	/* send email */
	send_email(email);

	return 0;

}

