# cmail_cli_app
A simple cli app for sending emails - only supports plain text / csv file attachments

Supports secure connections
Supports multile emails and multple cc address

Set your from email 'apperance' from the config.h file. 

Secret config is kept in enviroment variables and used like so
char* EMAIL_USER = getenv("EMAIL_USER");  account user
char* EMAIL_USERNAME = getenv("EMAIL_USERNAME");  account user name
char* EMAIL_PASS = getenv("EMAIL_PASS"); account password
char* EMAIL_SMTP = getenv("EMAIL_SMTP"); account server
 


## Help
### Sherlly's simple cmail 
-h or --help to view help
-t or --to_addr sets to address
-n or --to_name sets to name
-c or --cc_addr sets cc address
-s or --subject sets subject
-b or --body sets body
if you want attachment, both of these must be specified
-a or --attachment_path sets attachment_path
-an or --attachment_name sets attachment_path
Will assume attachment name is the same as the path if not specified
-f or --force will means if an attachment isn't found, then it will send the email without an attachment rather than crashing.



### Basic Useage
mailer -t "example@example.com" --to_name "Example Person" -s "Subject here" -b "Body here - Simple string" 

sent email - email to -->  Example Person with email: example@example.com

### or with attachment:

mailer -t "example@example.com" --to_name "Example Person" -s "Subject here" -b "Body here - Simple string" -a "/attachment/path/file.txt" -an "attachment.txt"

sent email - email to -->  Example Person with email: example@example.com

### or with CC address:

mailer -t "example@example.com" --to_name "Example Person" -s "Subject here" -c "cc_addr@new_server.com" -b "Body here - Simple string" -a "/attachment/path/file.txt" -an "attachment.txt"

sent email - email to -->  Example Person with email: example@example.com

### Sending to multiple people
Just comma seperate, for example 

mailer -t "example@example.com, danny@sherllymail.com" -n "Example group name" -s "Subject here" -b "Body here - Simple string" 

spaces get trimmed for all addresses 

