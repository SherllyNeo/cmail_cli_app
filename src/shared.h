#ifndef PRIM
#define PRIM

#define MAX_LEN 255
#define DELIM ","

#define MAX_ADDR_LENGTH 1000
#define MAX_ADDR_ALIAS_LENGTH 1000
#define MAX_ADDR_AMOUNT 50

#define MAX_ATTACH_LENGTH 1000
#define MAX_ATTACH_ALIAS_LENGTH 1000
#define MAX_ATTACH_AMOUNT 20

#define MAX_SUBJECT_LENGTH 200
#define MAX_BODY_LENGTH 500

#define SUBJECT_SIZE 500
#define BODY_SIZE 5000
#define TO_SIZE 200
#define FROM_SIZE 500
#define CC_SIZE 200
#define BCC_SIZE 200
#define ATTACHMENT_SIZE 5000000
#define TOTAL_MAX_ATTACH_SIZE ATTACHMENT_SIZE*MAX_ATTACH_AMOUNT
#define PAYLOAD_SIZE (SUBJECT_SIZE + BODY_SIZE + TOTAL_MAX_ATTACH_SIZE)*2

#define EXT_ATTACHMENT_ERR 5
#define EXT_EMAIL_ERR 6
#define EXT_CONNECTION_ERR 7

#define ERR_ENV_PARSING 8

#define EXT_EMAIL_INIT 1
#define EXT_ARG_PARSING 2
#define EXT_EMAIL_NULL 3



typedef enum {
    TO,
    CC,
    BCC
} AddressType;

typedef enum {
    TXT,
    CSV,
    JPG,
    PNG,
    PDF,
    UNKNOWN
} fileType;
typedef struct {
    char address[MAX_ADDR_LENGTH];
    char name[MAX_ADDR_ALIAS_LENGTH];
    AddressType addressType;
    bool valid;
} Address;

typedef struct {
    char filepath[MAX_ATTACH_LENGTH];
    char name[MAX_ATTACH_ALIAS_LENGTH];
    fileType filetype;
    bool valid;
} Attachment;

typedef struct {
    Address addresses[MAX_ADDR_AMOUNT];
    int amount_of_addresses;
    Address ccaddresses[MAX_ADDR_AMOUNT];
    int amount_of_ccaddresses;
    Address bccaddresses[MAX_ADDR_AMOUNT];
    int amount_of_bccaddresses;
    Attachment attachments[MAX_ATTACH_AMOUNT];
    int amount_of_attachments;
    char subject[MAX_SUBJECT_LENGTH];
    char body[MAX_SUBJECT_LENGTH];
    char fromUser[MAX_SUBJECT_LENGTH];
    char fromUsername[MAX_SUBJECT_LENGTH];
} Email;

#endif
