#!/bin/bash

file=email_list.csv
{
read
while IFS=, read -r address name ccaddress subject body attachment_path attachment_name; do
    mailer "$address" "$name" "$ccaddress" "$subject" "$body" "$attachment_path" "$attachment_name";
  done } < $file
