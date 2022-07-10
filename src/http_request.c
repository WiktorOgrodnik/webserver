/**
*	Wiktor Ogrodnik
*	323129
*/

#include "http_request.h"
#include <ctype.h>

static bool filter_url(char* url) {
	char* t = strchr(url, '.');
	if (t == NULL) return true;
	if (*(t + 1) == '.') { return false; }
	return true;
}

static void trim(char* source) {
	char* d = source;
    do {
        while (isspace(*d)) {
            ++d;
        }
    } while ((*source++ = *d++));
}

struct http_request_header* http_request_header_parse(char buffer[], int* error) {

	char* buffer_to_count_fileds = buffer;
	int num = -2;

	while (*buffer_to_count_fileds != '\0') {
		if (*buffer_to_count_fileds == '\n') num++;
		buffer_to_count_fileds++;
	}

	if (num <= 0) { *error = 501; return NULL;}

	struct http_request_header* header = (struct http_request_header*)malloc(sizeof(struct http_request_header) 
																				+ (size_t)num * sizeof(struct http_header_field));

	if (header == NULL) { *error = 500; return NULL;}

	header->number_of_fields = num;
	
	size_t first_line_len = strchr(buffer, '\n') - buffer - 1;

	char option[10];
	char protocol[10];
	char* path = (char*)malloc(first_line_len * sizeof(char));
	if (path == NULL) { *error = 500; return NULL; }

	sscanf(buffer, "%8s %s %9s", option, path, protocol);

	header->command = GET;
	header->url = (char*)malloc(strlen(path) * sizeof(char) + 1);
	header->protocol_ver = (char*)malloc(strlen(protocol) * sizeof(char) + 1);

	if (header->url == NULL || header->protocol_ver == NULL) {*error = 500; return NULL; }

	strcpy(header->url, path);
	strcpy(header->protocol_ver, protocol);

	if (!filter_url(header->url)) { *error = 403; return NULL; }

	buffer = strchr(buffer, '\n') + 1;

	for (size_t i = 0; i < header->number_of_fields; i++) {
		char* next_buffer = strchr(buffer, '\n');
		*(next_buffer++) = '\0';

		char* colon_buffer = strchr(buffer, ':');
		if (colon_buffer == NULL) { *error = 501; return NULL; }

		*(colon_buffer++) = '\0';
		if (*colon_buffer == ' ') colon_buffer++;

		header->fields[i].field_name = (char*)malloc(strlen(buffer) * sizeof(char) + 1);
		header->fields[i].field_content = (char*)malloc(strlen(colon_buffer) * sizeof(char) + 1);

		if (header->fields[i].field_name == NULL || header->fields[i].field_content == NULL) { *error = 500; return NULL; }

		strcpy(header->fields[i].field_name, buffer);
		strcpy(header->fields[i].field_content, colon_buffer);

		trim(header->fields[i].field_name);
		trim(header->fields[i].field_content);

		buffer = next_buffer;
	}
	
	return header;
}

void get_host(struct http_request_header* header, char* host_name) {

	for (size_t i = 0; i < header->number_of_fields; i++) {
		if (strcmp("Host", header->fields[i].field_name) == 0) {
			strcpy(host_name, header->fields[i].field_content);

			char* colon = strchr(host_name, ':');
			if (colon) *colon = '\0';

			return;
		}
	}

	strcpy(host_name, "none");
}

bool connection_to_close(struct http_request_header* header) {

	for (size_t i = 0; i < header->number_of_fields; i++) {
		if (strcmp("Connection", header->fields[i].field_name) == 0) {
			if (strcmp("close", header->fields[i].field_content) == 0) {
				return true;
			}
			return false;
		}
	}

	return false;
}
