#ifndef SHARED
#define SHARED
struct Email {
	char* To_addr;
	char* To_name;
	char* Cc_addr;
	char* Subject;
	char* Body;
	char* Attachment_path;
	char* Attachment_name;
};

#endif

