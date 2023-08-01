CC=gcc
cflags=-lcurl

cmail: cmail.c emailer.h config.h mail.c
	$(CC) -o mailer cmail.c emailer.h config.h $(cflags)

install:
	$(CC) -o mailer cmail.c emailer.h config.h $(cflags) && cp ./mailer /usr/local/bin
