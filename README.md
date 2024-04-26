# Simplex mail

## Getting started

### gmail example

Set these envrioment variables, recommended to use unix pass or similar to keep password safe
```bash
EMAIL_PASS="$(pass show emailPassword)"
EMAIL_SMTP=smtp://smtp.google.com:587
EMAIL_USERNAME="Real name"
EMAIL_USER="user@gmail.com"
```

In order to do this, you need to get an [app password from google, instructions here](https://support.google.com/accounts/answer/185833?hl=en)

However other mail boxes will let you log in with your actual passwords 

Then 
```bash
git clone https://github.com/SherllyNeo/cmail_cli_app.git
```
```bash
cd cmail_cli_app && make && make install
```

And you are done :) it installs to ~/.local/bin/mailer by default.

If you can an error for libcurl, you can find install instructions [here](https://ec.haxx.se/install/)

## Useage

### Example

```bash
  ____ ___ __  __ ____  _     _______  __  __  __    _    ___ _
/ ___|_ _|  \/  |  _ \| |   | ____\ \/ / |  \/  |  / \  |_ _| |
\___ \| || |\/| | |_) | |   |  _|  \  /  | |\/| | / _ \  | || |
 ___) | || |  | |  __/| |___| |___ /  \  | |  | |/ ___ \ | || |___
|____/___|_|  |_|_|   |_____|_____/_/\_\ |_|  |_/_/   \_\___|_____|
by SherllyNeo

mailer -s "Test Subject" -b "Test body" -t "user@example.com: Firstname Lastname,john.smith@email.com: John Smith" -a "README.md:New file.md,test.pdf" 
[+] Retrieved enviroment variables 
[+] Parsed addresses 
[+] Created email 
[+] Retrieved all 2 attachments content 
[+] sent email (Test Subject) at: Mon Apr 22 15:32:49 2024 
to: 
user@example.com 
john.smith@email.com 
[+] Done :) 
```
### Addresses and Attachments
You must specify them in a comma seperated string like so
```bash
mailer -t "john.smith@email.com,user1@example.com" -c "ccaddress@example.com,yetanother@domain.com"
```

### Requirements
You must specify the to addresses via -t or the blind carbon copy (BCC) addresses -bc. Just specifying carbon copy (CC) addresses will not work unless there are regular to addresses..

blind carbon copy (BCC) addresses are sent the email content without seeing any other addresses the email has been sent to.

You must have set your enviroment variables correctly for it to connect to your SMTP domain and send the email. 

That is it, there is no need to add attachments or aliases or anything else. You can send blank emails.

### Aliases
For both attachments and email addresses, you can alias how they will be seen by the reciever. 
Simpily add a : into your attachments like so. 
```bash
mailer -t "john.smith@email.com:Alias name" -a "~/Downloads/file.pdf:alias.pdf"
```
### Attachments
Most attachments are supported and it will dynamically add memory for larger files up to a limit defined in shared.h


## Philosophy 

Simple, Safe and elegant code. 

The goal here is to make a small functional app for sending basic emails without the need to open a browser or app.

This is useful for alerts from a server, simple mailing lists and more.

It is scriptable, UNIX friendly and extensible. 

## TODO

* Signal handling 
* Man page
* Portable version with cross platform compilation
* Retrieving mail from IMAP server in a scriptable way
* Using above to check if sending has failed on the server side










