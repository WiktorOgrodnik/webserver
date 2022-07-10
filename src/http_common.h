#ifndef HTTP_COMMON_H
#define HTTP_COMMON_H

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/stat.h>

enum http_code {
	OK = 0, MOVE_PERMAMENTLY, BAD_REQUEST, FORBIDDEN, NOT_FOUND, INTERNAL_SERVER_ERROR, NOT_IMPLEMENTED
};

extern char* http_code_string[];

enum http_method {
	GET = 0, HEAD, POST, PUT, DELETE, CONNECT, OPTIONS, TRACE, PATCH
};

extern char* http_method_string[];

struct http_header_field {
	char* field_name;
	char* field_content;
};

#define ERROR(str) { fprintf(stderr, "%s: %s\n", (str), strerror(errno)); exit(EXIT_FAILURE); }

int check(int res, const char* message);

#endif