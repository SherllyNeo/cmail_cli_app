#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "emailer.h"
#include "shared.h"




int main(int argc, char* argv[]) {

	if (argc<5) {
	printf("\nYou need at least 5 arguments. To Address, To name, CC address, Subject, Body and optional attachment path and name\n");
	fprintf(stderr,"\nToo few arguments, needs at least five \n");
	exit(0);
	}
	struct Email email;
	if (argv[6] && argv[7]) {
		email = (struct Email) {argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],argv[7]};
	}
	else {
		email = (struct Email) {argv[1],argv[2],argv[3],argv[4],argv[5],NULL,NULL};
	}

	/* send email */
	send_email(email);

	return 0;

}

