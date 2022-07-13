/**
*	Wiktor Ogrodnik
*	323129
*/

#include "http_request.h"
#include <ctype.h>

static void http_request_init_field(struct http_header_field* const field,
                             const char* const name, 
							 const char* const content);

static void http_request_insert_field(struct http_request_header* const header, 
							   const char* const name, 
							   const char* const content);

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

static size_t http_header_number_of_fields(const char* buffer) {

	int num = -2;

	while (*buffer != '\0') {
		if (*buffer == '\n') num++;
		buffer++;
	}

	return num < 0 ? 0ULL : (size_t)num;
}

static struct http_request_header* http_request_header_init(char* buffer, int* error) {

	size_t number_of_fields = http_header_number_of_fields(buffer);

	if (number_of_fields == 0) { *error = 501; return NULL; }

	struct http_request_header* header = (struct http_request_header*)malloc(sizeof(struct http_request_header) 
											+ (size_t)number_of_fields * sizeof(struct http_header_field));

	if (header == NULL) { *error = 500; return NULL;}


}

struct http_request_header* http_request_header_parse(char buffer[], int* error) {

	struct http_request_header* header = http_request_header_init(buffer, error);

	if (header == NULL) { *error = 500; return NULL;}

	header->number_of_fields = 0;
	
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

	for (int i = 0; i < number_of_fields; i++) {
		char* next_buffer = strchr(buffer, '\n');
		*(next_buffer++) = '\0';

		char* colon_buffer = strchr(buffer, ':');
		if (colon_buffer == NULL) { *error = 501; return NULL; }

		*(colon_buffer++) = '\0';
		if (*colon_buffer == ' ') colon_buffer++;

		http_request_insert_field(header, buffer, colon_buffer);

		buffer = next_buffer;
	}
	
	return header;
}

static void http_request_init_field(struct http_header_field* const field,
                             const char* const name, 
							 const char* const content) 
{
	if (field == NULL) return;

	if ((field->field_name = (char*)malloc((strlen(name) + 1) * sizeof(char))) == NULL)
		exit(EXIT_FAILURE);

	if ((field->field_content = (char*)malloc((strlen(content) + 1) * sizeof(char))) == NULL)
		exit(EXIT_FAILURE);

	strcpy(field->field_name, name); strcpy(field->field_content, content);

	trim(field->field_name); trim(field->field_content);
}

static void http_request_insert_field(struct http_request_header* const header, 
							   const char* const name, 
							   const char* const content) 
{
	
	if (header == NULL) return;

	size_t number_of_fields = header->number_of_fields;
	http_request_init_field(&header->fields[number_of_fields], name, content);

	header->number_of_fields++;
}

char* http_request_get_data(struct http_request_header* header, const char* const name) {

	for (size_t i = 0; i < header->number_of_fields; i++) {
		if (strcmp(name, header->fields[i].field_name) == 0) {
			return header->fields[i].field_content;
		}
	}

	return NULL;
}

bool http_request_content_equal(struct http_request_header* header, 
								const char* const name, 
								const char* const content)
{
	char* real_content = http_request_get_data(header, name);
	return real_content != NULL && strcmp(content, real_content) == 0;
}

char* http_request_get_host(struct http_request_header* header) {

	char* real_host = http_request_get_data(header, "Host");
	
	if (real_host) {
		char* text;
		if ((text = (char*)malloc(strlen(real_host))) == NULL) { exit(EXIT_FAILURE); }

		strcpy(text, real_host);

		char* colon = strchr(text, ':');
		if (colon) *colon = '\0';

		return text;
	}

	return NULL;
}
