CC=gcc
cflags=-lcurl

cmail: cmail.c emailer.h config.h mail.c
	$(CC) -o mailer cmail.c emailer.h config.h $(cflags)
