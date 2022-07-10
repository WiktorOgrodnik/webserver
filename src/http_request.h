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
void get_host(struct http_request_header* header, char* host_name);
bool connection_to_close(struct http_request_header* header);

#endif