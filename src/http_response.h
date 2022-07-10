#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_request.h"

struct http_response_header {
	enum http_code code;
	size_t number_of_fields;
	struct http_header_field fields[];
};


void http_response_send_file(struct stat* st, struct http_request_header* http_header, const char* filename, int client_socket);
void http_response_send_redirect(struct http_request_header* http_header, int client_socket);
void http_response_send_not_found(int client_socket);
void http_response_send_forbidden(int client_socket);
void http_response_send_not_implemented(int client_socket);

#endif