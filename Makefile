CFLAGS = -g3 -O -Wall -W -pedantic -std=c99
LDFLAGS = -L/opt/vc/lib
LDLIBS =  -lcurl

mailer src/main.c: 
	gcc $(CFLAGS) $(LDFLAGS) src/main.c -o bin/mailer $(LDLIBS)
install: src/main.c src/main.c src/mail.c src/mail.h src/shared.h src/composer.h src/composer.c src/parser.c src/parser.h 
	gcc $(CFLAGS) $(LDFLAGS) src/main.c -o bin/mailer $(LDLIBS) && cp -f ./bin/mailer $(HOME)/.local/bin/
