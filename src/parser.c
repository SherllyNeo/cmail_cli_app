#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include "shared.h"
#include <wordexp.h>



void remove_spaces (char* str_trimmed, char* str_untrimmed)
{
  while (*str_untrimmed != '\0')
  {
    if(!isspace(*str_untrimmed))
    {
      *str_trimmed = *str_untrimmed;
      str_trimmed++;
    }
    str_untrimmed++;
  }
  *str_trimmed = '\0';
}

char *trimwhitespace(char *str)
{
  char *end;

  while(isspace((unsigned char)*str)) str++;

  if(*str == 0) 
    return str;

  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  end[1] = '\0';

  return str;
}

bool validate_email_address(char* emailAddr) {
    bool valid = false;
    if (emailAddr == NULL) {
        return valid;
    }
    if (strlen(emailAddr) < 5) {
        return valid;
    }

    if(strchr(emailAddr, '@') == NULL) {
        return valid;
    }
    if(strchr(emailAddr, '.') == NULL) {
        return valid;
    }

    valid = true;
    return valid;
}

int countCommas(const char *str) {
    int count = 0;
    if (str == NULL) {
        return count;
    }
    while (*str != '\0') {
        if (*str == ',') {
            count++;
        }
        str++;
    }
    return count;
}

Address parserInitAddress(char* addressStr, char* nameStr, char* addressTypeStr) {
    Address returnAddress; 
    AddressType addressType;
    returnAddress.valid = false;
    char tmp[MAX_ADDR_LENGTH];

    remove_spaces(tmp, addressStr);

    if (strlen(addressStr) > (MAX_ADDR_LENGTH - 1)) {
        fprintf(stderr, "\nERROR: address is too long: %s\n", addressStr);
        return returnAddress;
    }
    if (strlen(nameStr) > (MAX_ADDR_ALIAS_LENGTH - 1)) {
        fprintf(stderr, "\nERROR: name is too long: %s\n", addressStr);
        return returnAddress;
    }

    if (!strcmp(addressTypeStr,"TO")) {
            addressType = TO;
    }
    else if (!strcmp(addressTypeStr,"CC")) {
            addressType = CC;
    }
    else if (!strcmp(addressTypeStr,"BCC")) {
            addressType = BCC;
    }
    else {
        fprintf(stderr, "\nERROR: unknown addressType: %s\nfor address: %s\n", tmp,addressStr);
        return returnAddress;
    }

    if (!validate_email_address(tmp)) {
        fprintf(stderr, "\nERROR: Address %s failed validation",tmp);
        strcpy(returnAddress.address,tmp);
        strcpy(returnAddress.name,nameStr);
        returnAddress.addressType = addressType;
        return returnAddress;
    }

    strcpy(returnAddress.address,tmp);
    strcpy(returnAddress.name,nameStr);
    returnAddress.addressType = addressType;
    returnAddress.valid = true;

    return returnAddress;
}

void printAddress(Address addr) {
    printf("address: %s, name: %s, type: %d, valid: %d\n",addr.address,addr.name,addr.addressType,addr.valid);
}
void get_file_type(const char* filename, char* typebuffer, size_t buffer_size) {
    const char* file_extension = strrchr(filename, '.');
    if (file_extension != NULL) {
        strncpy(typebuffer, file_extension + 1, buffer_size - 1); 
        typebuffer[buffer_size - 1] = '\0'; 
    } else {
        strncpy(typebuffer, "UNKNOWN", buffer_size - 1); 
        typebuffer[buffer_size - 1] = '\0';
    }
    int i = 0;
    while (typebuffer[i] != '\0') {
        typebuffer[i] = toupper(typebuffer[i]);
        i++;
    }
}

Attachment parserInitAttachment(char* filepathStr, char* nameStr) {
    Attachment returnAttachment; 
    fileType AttachmentType;
    char cleaned_filepath_str[MAX_ADDR_LENGTH] = { 0 };
    returnAttachment.valid = false;

    wordexp_t exp_result;
    wordexp(filepathStr, &exp_result, 0);
    sprintf(cleaned_filepath_str,"%s", exp_result.we_wordv[0]);
    wordfree(&exp_result);

    if (strlen(filepathStr) > (MAX_ATTACH_LENGTH - 1)) {
        fprintf(stderr, "\nERROR: attachment name is too long: %s\n", filepathStr);
        return returnAttachment;
    }
    if (strlen(nameStr) > (MAX_ADDR_ALIAS_LENGTH - 1)) {
        fprintf(stderr, "\nERROR: name is too long: %s\n", filepathStr);
        return returnAttachment;
    }

    /* get file name */
    if (strlen(nameStr) == 0) {
        nameStr = basename(filepathStr);
    }



    /* get file attachment */
    char attachmentTypeStr[MAX_LEN];
    get_file_type(cleaned_filepath_str, attachmentTypeStr, sizeof(attachmentTypeStr));

    char attachmentTypeStrFromName[MAX_LEN];
    get_file_type(nameStr, attachmentTypeStrFromName, sizeof(attachmentTypeStrFromName));

    /* ensure file types agree */
    if (strcmp(attachmentTypeStrFromName,attachmentTypeStr) != 0) {
        fprintf(stderr, "\nERROR: filepath %s and filename inputted %s do not match file type %s\n",cleaned_filepath_str,nameStr,attachmentTypeStr);
        return returnAttachment;
    }

    /* Check if path exists */
    FILE *file;
    if((file = fopen(cleaned_filepath_str,"r"))!=NULL)
        {
            fclose(file);
        }
    else
        {
        fprintf(stderr, "\nERROR: filepath %s cannot be opened so likely does not exist\n",cleaned_filepath_str);
        return returnAttachment;
        }

    if (!strcmp(attachmentTypeStr,"TXT")) {
        AttachmentType = TXT;
    }
    else if (!strcmp(attachmentTypeStr,"CSV")) {
        AttachmentType = CSV;
    }
    else if (!strcmp(attachmentTypeStr,"JPG")) {
        AttachmentType = JPG;
    }
    else if (!strcmp(attachmentTypeStr,"PNG")) {
        AttachmentType = PNG;
    }
    else if (!strcmp(attachmentTypeStr,"PDF")) {
        AttachmentType = PDF;
    }
    else {
        AttachmentType = UNKNOWN;
    }

    strcpy(returnAttachment.filepath,cleaned_filepath_str);
    strcpy(returnAttachment.name,nameStr);
    returnAttachment.filetype = AttachmentType;
    returnAttachment.valid = true;

    return returnAttachment;
}

void printAttachment(Attachment at) {
    printf("filepath: %s, filename: %s, type: %d, valid: %d\n",at.filepath,at.name,at.filetype,at.valid);
}




void printEmail(Email e) {
    printf("sending to:\n");
    for (int i = 0; i<e.amount_of_addresses; i++) {
        printAddress(e.addresses[i]);
    }
    printf("\n");
    printf("cc-ing in:\n");
    for (int j = 0; j<e.amount_of_ccaddresses; j++) {
        printAddress(e.ccaddresses[j]);
    }
    printf("\n");
    printf("blind cc-ing in:\n");
    for (int k = 0; k<e.amount_of_bccaddresses; k++) {
        printAddress(e.bccaddresses[k]);
    }
    printf("\n");
    printf("with attachments:\n");
    for (int z = 0; z<e.amount_of_attachments; z++) {
        printAttachment(e.attachments[z]);
    }
    printf("\nSubject: %s\nBody: %s\n",e.subject,e.body);
}




/* PUBLIC FUNCTIONS - preappended with parser to show which file it came from */

Email parserInitEmail(char* fromUser, char* fromUsername, Address addresses[MAX_ADDR_AMOUNT],int amount_of_addresses,Address ccaddresses[MAX_ADDR_AMOUNT],int amount_of_ccaddresses,Address bccaddresses[MAX_ADDR_AMOUNT],int amount_of_bccaddresses,Attachment attachments[MAX_ATTACH_AMOUNT],int amount_of_attachments,char subject[MAX_SUBJECT_LENGTH],char body[MAX_SUBJECT_LENGTH]) {
    Email returnEmail;

    returnEmail.amount_of_addresses = amount_of_addresses;
    returnEmail.amount_of_ccaddresses = amount_of_ccaddresses;
    returnEmail.amount_of_bccaddresses = amount_of_bccaddresses;
    returnEmail.amount_of_attachments = amount_of_attachments;

    for (int i = 0; i<amount_of_addresses; i++) {
        returnEmail.addresses[i] = addresses[i];
    }
    for (int j = 0; j<amount_of_ccaddresses; j++) {
        returnEmail.ccaddresses[j] = ccaddresses[j];
    }
    for (int k = 0; k<amount_of_bccaddresses; k++) {
        returnEmail.bccaddresses[k] = bccaddresses[k];
    }
    for (int z = 0; z<amount_of_attachments; z++) {
        returnEmail.attachments[z] = attachments[z];
    }

    strcpy(returnEmail.subject,subject);
    strcpy(returnEmail.body,body);
    strcpy(returnEmail.fromUser,fromUser);
    strcpy(returnEmail.fromUsername,fromUsername);

    return returnEmail;
}

int parserParseAddresses(const char* input,Address addresses[MAX_ADDR_AMOUNT], char* type) {
    int valid_addresses = 0;
    int num_addresses = countCommas(input) + 1;
    memset(addresses,0,sizeof(Address)*num_addresses+1);

    int address_index = 0;
    char* token;
    char* copy = malloc(sizeof(char)*strlen(input)*2);
    strcpy(copy,input);


    token = strtok(copy, ",");

    while (token != NULL) {
        char* separator = strchr(token, ':');
        char* name = "";
        Address result;
        if (separator != NULL) {
            *separator = '\0'; 
            name = separator + 1;
        }
        
        result = parserInitAddress(token, name, type);


        if (!result.valid) {
            fprintf(stderr,"\nSKIPPING: issue with address %s\n",token);
            token = strtok(NULL, ",");
            continue;
        }
        else {
            addresses[address_index] = result;
            valid_addresses++;
        }
        address_index++;
        token = strtok(NULL, ",");
    }
    free(copy);
    return valid_addresses;
}

int parserParseAttachments(const char* input,Attachment attachments[MAX_ATTACH_AMOUNT]) {
    /* returns the amount of valid attachments */
    int valid_attachments = 0;
    int num_attachments = countCommas(input) + 1;
    memset(attachments,0,sizeof(Attachment)*num_attachments+1);

    int attachment_index = 0;
    char* token;
    char* copy = malloc(sizeof(char)*strlen(input)*2);
    strcpy(copy,input);


    token = strtok(copy, ",");

    while (token != NULL) {
        char* separator = strchr(token, ':');
        char* name = "";
        Attachment result;
        if (separator != NULL) {
            *separator = '\0'; 
            name = separator + 1;
        }

        result = parserInitAttachment(token, name);
        if (!result.valid) {
            fprintf(stderr,"\nSKIPPING: issue with attachment %s, skipping\n",token);
            token = strtok(NULL, ",");
            continue;
        }
        else {
            attachments[attachment_index] = result;
            valid_attachments++;
        }
        attachment_index++;
        token = strtok(NULL, ",");
    }
    free(copy);
    return valid_attachments;
}
