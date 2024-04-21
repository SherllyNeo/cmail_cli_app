# Simplex mail

## Getting started

### gmail example

Set these envrioment variables, recommended to use unix pass or similar to keep password safe
```bash
EMAIL_PASS="$(pass show emailPassword)"
EMAIL_SMTP=smtp://smtp.google.com:587
EMAIL_USERNAME="Real name"
EMAIL_USER="user@gmail..com"
```

In order to do this, you need to get an app password from google:
https://support.google.com/accounts/answer/185833?hl=en

However other mail boxes will let you log in with your actual passwords 

Then 
```bash
git clone https://github.com/SherllyNeo/cmail_cli_app.git
```
```bash
cd cmail_cli_app && mkdir bin && make && make install
```

And you are done :) it installs to ~/.local/bin/mailer by default.

## Useage


```bash
mailer -s "Test Subject" -b "Test body" -t "user@example.com: Firstname Lastname,john.smith@email.com: John Smith" -a "README.md:New file.txt,README.md"


  ____ ___ __  __ ____  _     _______  __  __  __    _    ___ _
/ ___|_ _|  \/  |  _ \| |   | ____\ \/ / |  \/  |  / \  |_ _| |
\___ \| || |\/| | |_) | |   |  _|  \  /  | |\/| | / _ \  | || |
 ___) | || |  | |  __/| |___| |___ /  \  | |  | |/ ___ \ | || |___
|____/___|_|  |_|_|   |_____|_____/_/\_\ |_|  |_/_/   \_\___|_____|


[+] Retrieved enviroment variables

ERROR: filepath README.md and filename inputted New file.txt do not match file type MD

SKIPPING: issue with attachment README.md, skipping
[+] Parsed addresses
[+] Created email
[+] Retrieved all 1 attachments content
[+] sent email (Test Subject) at: Sun Apr 21 22:49:33 2024
to:
user@example.com
john.smith@email.com

[+] Done :)
```

## Philosophy 

Simple, Safe and elegant code. 

The goal here is to make a small functional app for sending basic emails without the need to open a browser or app.

It is scriptable and UNIX friendly and extensible. 









