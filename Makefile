CC=gcc
cflags=-lcurl

cmail: cmail.c emailer.h mail.c
	$(CC) -o mailer cmail.c emailer.h $(cflags)

install:
	$(CC) -o mailer cmail.c emailer.h $(cflags) && cp ./mailer /usr/local/bin
