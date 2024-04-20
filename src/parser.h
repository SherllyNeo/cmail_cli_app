#ifndef PARSER
#define PARSER

#include "parser.c"
#include "primitives.h"



Address initAddress(char* addressStr, char* nameStr, char* addressTypeStr);
void printAddress(Address addr); 

Attachment initAttachment(char* filepathStr, char* nameStr);

void printAttachment(Attachment at);



int parseAddresses(const char* input,Address addresses[MAX_ADDR_AMOUNT], char* type);
int parseAttachments(const char* input,Attachment attachments[MAX_ATTACH_AMOUNT]);


void printEmail(Email e);

Email initEmail(char* fromUser, char* fromUsername, Address addresses[MAX_ADDR_AMOUNT],int amount_of_addresses,Address ccaddresses[MAX_ADDR_AMOUNT],int amount_of_ccaddresses,Address bccaddresses[MAX_ADDR_AMOUNT],int amount_of_bccaddresses,Attachment attachments[MAX_ATTACH_AMOUNT],int amount_of_attachments,char subject[MAX_SUBJECT_LENGTH],char body[MAX_SUBJECT_LENGTH]);

#endif
