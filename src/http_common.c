/**
*	Wiktor Ogrodnik
*	323129
*/

#include "http_common.h"

char* http_code_string[] = {"200 OK", "301 Moved Permanently", "400 Bad Request", "403 Forbidden", "404 Not Found", "500 Internal Server Error", "501 Not Implemented"};
char* http_method_string[] = {"GET", "HEAD", "POST", "PUT", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH"};

int check(int res, const char* message) {
	if (res < 0) ERROR(message);
	return res;
}