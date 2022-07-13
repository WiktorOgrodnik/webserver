/**
*	Wiktor Ogrodnik
*	323129
*/

#include "http_request.h"
#include <ctype.h>

/** Auxilary functions **/

static bool filter_url(char* url);
static void trim(char* source);
static char* next_word(char* buffer, int separator, int* error);

/** Internal http_request functions **/

static size_t http_header_number_of_fields(const char* buffer);

static struct http_request_header* http_request_header_init(char* buffer, 
													int* error, 
													size_t* number_of_fields);

static char* http_request_retrieve_first_line(struct http_request_header* header, char* buffer, int* error);

static char* http_request_internal_get_host(struct http_request_header* header, int* error);

static void http_request_init_field(struct http_header_field* const field,
                             const char* const name, 
							 const char* const content);

static void http_request_insert_field(struct http_request_header* const header, 
							   const char* const name, 
							   const char* const content);


/** Definisions **/

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

static char* next_word(char* buffer, int separator, int* error) {

	char* next = strchr(buffer, separator);
	if (next == NULL) *error = 501;
	return next;
}

static size_t http_header_number_of_fields(const char* buffer) {

	int num = -2;

	while (*buffer != '\0') {
		if (*buffer == '\n') num++;
		buffer++;
	}

	return num < 0 ? 0ULL : (size_t)num;
}

static struct http_request_header* http_request_header_init(char* buffer, 
													int* error, 
													size_t* number_of_fields)
{

	size_t num = http_header_number_of_fields(buffer);

	if (num == 0) { *error = 501; return NULL; }

	*number_of_fields = num;

	struct http_request_header* header = malloc(sizeof(struct http_request_header) 
									            + (size_t)num * sizeof(struct http_header_field));

	if (header == NULL) { *error = 500; return NULL;}

	header->number_of_fields = 0;

	return header;
}

// returns pointer to next line
static char* http_request_retrieve_first_line(struct http_request_header* header, char* buffer, int* error) {

	char* end_line = next_word(buffer, '\n', error);
	if (end_line) *(end_line++) = '\0'; else return NULL;
	
	char* option = buffer;
	
	char* path = next_word(buffer, ' ', error);
	if (path) *(path++) = '\0'; else return NULL;

	char* protocol = next_word(path, ' ', error);
	if (protocol) *(protocol++) = '\0'; else return NULL;

	/* Command sanitize */

	// TO REPLACE WITH OTHER SOLUTION
	if (strcmp(option, "GET") != 0) {*error = 501; return NULL; } // Temporary patch, other functions than GET does not work now
	header->command = GET;

	/* URL sanitize */

	if (!filter_url(path)) { *error = 403; return NULL; }

	if ((header->url = malloc((protocol - path) * sizeof(char))) == NULL) { *error = 500; return NULL; }
	strcpy(header->url, path);

	/* Protocol sanitize */

	if ((header->protocol_ver = malloc((end_line - protocol) * sizeof(char))) == NULL) { *error = 500; return NULL; }
	strcpy(header->protocol_ver, protocol);

	return end_line;
}

static char* http_request_internal_get_host(struct http_request_header* header, int* error) {

	char* real_host = http_request_get_data(header, "Host");
	
	if (real_host == NULL) { *error = 501; return NULL; }

	char* text;
	if ((text = malloc(strlen(real_host))) == NULL) { *error = 500; return NULL; }

	strcpy(text, real_host);

	char* colon = next_word(text, ':', error);
	if (colon) *colon = '\0';

	return text;
}

static void http_request_init_field(struct http_header_field* const field,
                             const char* const name, 
							 const char* const content) 
{
	if (field == NULL) return;

	if ((field->field_name = malloc((strlen(name) + 1) * sizeof(char))) == NULL)
		exit(EXIT_FAILURE);

	if ((field->field_content = malloc((strlen(content) + 1) * sizeof(char))) == NULL)
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

struct http_request_header* http_request_header_parse(char buffer[], int* error) {

	struct http_request_header* header = NULL;
	size_t number_of_fields = 0;
	
	if ((header = http_request_header_init(buffer, error, &number_of_fields)) == NULL) return NULL;	
	if ((buffer = http_request_retrieve_first_line(header, buffer, error)) == NULL) return NULL;

	for (size_t i = 0; i < number_of_fields; i++) {
		
		char* name = buffer;

		char* content = next_word(name, ':', error);
		if (content) *(content++) = '\0'; else return NULL;

		buffer = next_word(content, '\n', error);
		if (buffer) *(buffer++) = '\0'; else return NULL;

		http_request_insert_field(header, name, content);
	}

	char* host = http_request_internal_get_host(header, error);
	if (host) header->host = host; else { *error = 501; return NULL; }
	
	return header;
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
	return header->host;
}

void http_request_header_destroy(struct http_request_header* header) {
	free(header->url);
	free(header->host);
	free(header->protocol_ver);

	for (size_t i = 0; i < header->number_of_fields; i++) {
		free(header->fields[i].field_name);
		free(header->fields[i].field_content);
	}
}
