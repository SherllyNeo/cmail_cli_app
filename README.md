# cmail_cli_app
A simple cli app for sending emails - only supports plain text / csv file attachments


Useage

mailer "recipient@example.com" "FirstName LastName" "cc_ddr@example.com" "Subject of email goes here" "body of email goes here"

or for attachments
mailer "recipient@example.com" "FirstName LastName" "cc_ddr@example.com" "Subject of email goes here" "body of email goes here" "path/to/file" "name_you_want_to_show_for_File"
