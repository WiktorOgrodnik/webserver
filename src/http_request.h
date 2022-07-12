#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "http_common.h"

struct http_request_header {
	char* url;
	char* protocol_ver;
	size_t number_of_fields;
	enum http_method command;
	struct http_header_field fields[];
};

struct http_request_header* http_request_header_parse(char buffer[], int* error);
char* http_request_get_data(struct http_request_header* header, const char* const name);
bool http_request_content_equal(struct http_request_header* header, const char* const name, const char* const content);
char* http_request_get_host(struct http_request_header* header);

#endif