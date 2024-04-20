CFLAGS = -g3 -O -Wall -W -pedantic -std=c99 
LDFLAGS = -L/opt/vc/lib
LDLIBS =  -lcurl

mailer src/main.c:
	gcc $(CFLAGS) $(LDFLAGS) src/main.c -o bin/mailer $(LDLIBS)
install: src/main.c 
	gcc $(CFLAGS) $(LDFLAGS) src/main.c -o bin/mailer $(LDLIBS) && cp -f ./bin/mailer $(HOME)/.local/bin/
