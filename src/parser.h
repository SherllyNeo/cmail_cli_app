#ifndef PARSER
#define PARSER

#include "parser.c"
#include "shared.h"

int parserParseAddresses(const char* input,Address addresses[MAX_ADDR_AMOUNT], char* type);
int parserParseAttachments(const char* input,Attachment attachments[MAX_ATTACH_AMOUNT]);

#endif
