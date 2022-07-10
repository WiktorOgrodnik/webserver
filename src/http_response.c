/**
*	Wiktor Ogrodnik
*	323129
*/

#include "http_response.h"

static struct http_response_header* http_response_header_init(int* error, enum http_code code) {
	struct http_response_header* response_header = (struct http_response_header*)malloc(sizeof(struct http_response_header));

	if (response_header == NULL) { *error = 500; return NULL; }

	response_header->number_of_fields = 0;
	response_header->code = code;

	return response_header;
}

static void http_response_header_add_field(struct http_response_header** header, const char* field_name, const char* field_content, int* error) {

	struct http_response_header* header_ptr = NULL;

	size_t number_of_fields = (*header)->number_of_fields + 1;
	header_ptr = (struct http_response_header*)realloc((*header), sizeof(struct http_response_header) + number_of_fields * sizeof(struct http_header_field));

	if (header_ptr == NULL) { *error = 500; return; }

	header_ptr->number_of_fields = number_of_fields;

	header_ptr->fields[number_of_fields - 1].field_name = (char*)malloc(sizeof(char) * (strlen(field_name) + 1));
	header_ptr->fields[number_of_fields - 1].field_content = (char*)malloc(sizeof(char) * (strlen(field_content) + 1));

	if (header_ptr->fields[number_of_fields - 1].field_name == NULL || 
	    header_ptr->fields[number_of_fields - 1].field_content == NULL) {
	
		*error = 500; return;
	}

	strcpy(header_ptr->fields[number_of_fields - 1].field_name, field_name);
	strcpy(header_ptr->fields[number_of_fields - 1].field_content, field_content);

	*header = header_ptr;
}

static size_t http_response_header_compose(u_int8_t* buffer, struct http_response_header* header, u_int8_t* site_content, size_t site_size) {

	buffer[0] = '\0';
	char line[1000];
	sprintf(line, "HTTP/1.1 %s\r\n", http_code_string[header->code]);
	strcat((char*)buffer, line);

	for (size_t i = 0; i < header->number_of_fields; i++) {
		sprintf(line, "%s: %s\r\n", header->fields[i].field_name, header->fields[i].field_content);
		strcat((char*)buffer, line);
	}
	strcat((char*)buffer, "\r\n");

	size_t buffer_len = strlen((char*)buffer);
	memcpy(buffer + buffer_len, site_content, site_size);
	memcpy(buffer + buffer_len + site_size, "\r\n\r\n", 4);

	return buffer_len + site_size + 4;
}

static void get_content_type(char* content_type, char* url) {
	char* dot_prev = NULL;
	char* dot = url;

	while (true) {
		dot_prev = dot;
		dot = strchr(dot, '.');
		if (dot != NULL) dot++;
		else break;
	};

	dot = dot_prev;

	if (dot) {
		if (strcmp(dot, "txt") == 0) sprintf(content_type, "text/plain; charset=utf-8");
		else if (strcmp(dot, "html") == 0) sprintf(content_type, "text/html; charset=utf-8");
		else if (strcmp(dot, "css") == 0) sprintf(content_type, "text/css; charset=utf-8");
		else if (strcmp(dot, "jpg") == 0) sprintf(content_type, "image/jpeg");
		else if (strcmp(dot, "jpeg") == 0) sprintf(content_type, "image/jpeg");
		else if (strcmp(dot, "png") == 0) sprintf(content_type, "image/png");
		else if (strcmp(dot, "pdf") == 0) sprintf(content_type, "application/pdf");
		else sprintf(content_type, "application/octet-stream");
	} else sprintf(content_type, "application/octet-stream");
}

static void get_system_date(char* date_time) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	strftime(date_time, sizeof(date_time) - 1, "%d %m %Y %H:%M", t);
}

static void get_file_length(size_t site_size, char* site_size_text) {
	sprintf(site_size_text, "%lu", site_size);
}

void http_response_send_file(struct stat* st, struct http_request_header* http_header, const char* filename, int client_socket) {

	int error = 0;
	char content_type[60];
	char site_size_text[20];
	char date_time[80];

	struct http_response_header* response_header = http_response_header_init(&error, OK);

	if (error == 500) { exit(EXIT_FAILURE); }
	
	size_t site_size = st->st_size;

	get_system_date(date_time);
	get_content_type(content_type, http_header->url);
	get_file_length(site_size, site_size_text);

	http_response_header_add_field(&response_header, "Date", date_time, &error);
	http_response_header_add_field(&response_header, "Server", "My own C Server for edu reasons: 0.0.1", &error);
	http_response_header_add_field(&response_header, "Accept-Ranges", "none", &error);
	http_response_header_add_field(&response_header, "Content-Type", content_type, &error);
	http_response_header_add_field(&response_header, "Content-Length", site_size_text, &error);

	u_int8_t* site_content = (u_int8_t*)malloc(sizeof(u_int8_t) * site_size);

	if (site_content == NULL) { exit(EXIT_FAILURE); }
				
	FILE* site_file = fopen(filename, "rb");
	fread(site_content, sizeof(char), site_size, site_file);
	fclose(site_file);

	u_int8_t* reply_buffer = (u_int8_t*)malloc(sizeof(u_int8_t) * (site_size + 5000));
	size_t reply_size = http_response_header_compose(reply_buffer, response_header, site_content, site_size);

	check((send(client_socket, reply_buffer, reply_size, 0)), "send error");

	free(site_content);
	free(reply_buffer);

	for (size_t i = 0; i < response_header->number_of_fields; i++) {
		free(response_header->fields[i].field_name);
		free(response_header->fields[i].field_content);
	}

	free(response_header);
}

void http_response_send_redirect(struct http_request_header* http_header, int client_socket) {
	int error = 0;
	struct http_response_header* response_header = http_response_header_init(&error, MOVE_PERMAMENTLY);

	if (error == 500) { exit(EXIT_FAILURE); }

	strcat(http_header->url, "index.html");
	http_response_header_add_field(&response_header, "Location", http_header->url, &error);

	u_int8_t* reply_buffer = (u_int8_t*)malloc(sizeof(u_int8_t) * 500);
	if (reply_buffer == NULL) { exit(EXIT_FAILURE); }

	size_t reply_size = http_response_header_compose(reply_buffer, response_header, NULL, 0);

	check((send(client_socket, reply_buffer, reply_size, 0)), "send error");

	free(reply_buffer);

	for (size_t i = 0; i < response_header->number_of_fields; i++) {
		free(response_header->fields[i].field_name);
		free(response_header->fields[i].field_content);
	}

	free(response_header);
}

void http_response_send_not_found(int client_socket) {

	int error = 0;
	struct http_response_header* response_header = http_response_header_init(&error, NOT_FOUND);

	if (error == 500) { exit(EXIT_FAILURE); }

	char date_time[100];
	char message[] = "404 Not Found\r\n";
	get_system_date(date_time);

	http_response_header_add_field(&response_header, "Date", date_time, &error);
	http_response_header_add_field(&response_header, "Server", "My own C Server for edu reasons: 0.0.1", &error);
	http_response_header_add_field(&response_header, "Content-Type", "text/plain; charset=utf-8", &error);
	http_response_header_add_field(&response_header, "Content-Length", "15", &error);

	u_int8_t* reply_buffer = (u_int8_t*)malloc(sizeof(u_int8_t) * 500);
	if (reply_buffer == NULL) { exit(EXIT_FAILURE); }

	size_t reply_size = http_response_header_compose(reply_buffer, response_header, (u_int8_t*)message, 15);

	check((send(client_socket, reply_buffer, reply_size, 0)), "send error");

	free(reply_buffer);

	for (size_t i = 0; i < response_header->number_of_fields; i++) {
		free(response_header->fields[i].field_name);
		free(response_header->fields[i].field_content);
	}

	free(response_header);
}

void http_response_send_forbidden(int client_socket) {

	int error = 0;
	struct http_response_header* response_header = http_response_header_init(&error, FORBIDDEN);

	if (error == 500) { exit(EXIT_FAILURE); }

	char date_time[100];
	char message[] = "403 Forbidden\r\n";
	get_system_date(date_time);

	http_response_header_add_field(&response_header, "Date", date_time, &error);
	http_response_header_add_field(&response_header, "Server", "My own C Server for edu reasons: 0.0.1", &error);
	http_response_header_add_field(&response_header, "Content-Type", "text/plain; charset=utf-8", &error);
	http_response_header_add_field(&response_header, "Content-Length", "15", &error);

	u_int8_t* reply_buffer = (u_int8_t*)malloc(sizeof(u_int8_t) * 500);
	if (reply_buffer == NULL) { exit(EXIT_FAILURE); }

	size_t reply_size = http_response_header_compose(reply_buffer, response_header, (u_int8_t*)message, 15);

	check((send(client_socket, reply_buffer, reply_size, 0)), "send error");

	free(reply_buffer);

	for (size_t i = 0; i < response_header->number_of_fields; i++) {
		free(response_header->fields[i].field_name);
		free(response_header->fields[i].field_content);
	}

	free(response_header);
}

void http_response_send_not_implemented(int client_socket) {

	int error = 0;
	struct http_response_header* response_header = http_response_header_init(&error, NOT_IMPLEMENTED);

	if (error == 500) { exit(EXIT_FAILURE); }

	char date_time[100];
	get_system_date(date_time);

	http_response_header_add_field(&response_header, "Date", date_time, &error);
	http_response_header_add_field(&response_header, "Server", "My own C Server for edu reasons: 0.0.1", &error);

	u_int8_t* reply_buffer = (u_int8_t*)malloc(sizeof(u_int8_t) * 500);
	if (reply_buffer == NULL) { exit(EXIT_FAILURE);}

	size_t reply_size = http_response_header_compose(reply_buffer, response_header, NULL, 0);

	check((send(client_socket, reply_buffer, reply_size, 0)), "send error");

	free(reply_buffer);

	for (size_t i = 0; i < response_header->number_of_fields; i++) {
		free(response_header->fields[i].field_name);
		free(response_header->fields[i].field_content);
	}

	free(response_header);
}