#include "http_response.h"
#include <stdint.h>

char* catalog_name;

#define BUFFER_SIZE 10000000
#define CONNECTION_TIMEOUT 1000

static int init_server(u_int16_t port, int backlog) {
	int server_socket;
	struct sockaddr_in server_addr;

	check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "Socket error");

	bzero(&server_addr, sizeof(server_addr));
	server_addr = (struct sockaddr_in){ .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(port)};

	check(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)), "bind error");
	check(listen(server_socket, backlog), "listen error");

	return server_socket;
}

static int accept_connection(int server_socket) {
	int client_socket;
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);
	check((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len)), "accept error");

	return client_socket;
}

static bool incomplete_http_request(u_int8_t* buffer, ssize_t buffer_size) {

	if (buffer_size == 0) return true;

	uintptr_t end_of_http = (uintptr_t)strstr((char*)buffer, "\r\n\r\n") | (uintptr_t)strstr((char*)buffer, "\n\n");

	return end_of_http == 0; //return true if \r\n\r\n not found, the message is incomplete
}

void* handle_connection(void* data) {

	int client_socket = *(int*)data;
	free(data);

	struct timeval tv; tv.tv_sec = CONNECTION_TIMEOUT; tv.tv_usec = 0;

	for (;;) {

		u_int8_t* recv_buffer = (u_int8_t*)malloc(BUFFER_SIZE);
		u_int8_t* temporary_buffer = (u_int8_t*)malloc(BUFFER_SIZE);

		ssize_t bytes_read = 0;
		int ready;
		
		while (incomplete_http_request(recv_buffer, bytes_read)) {

			fd_set descriptors;
			FD_ZERO(&descriptors);
			FD_SET(client_socket, &descriptors);

			check((ready = select(client_socket + 1, &descriptors, NULL, NULL, &tv)), "select error");
			if (ready == 0) break;

			ssize_t temp_bytes_read = 0;
			check((temp_bytes_read = recv(client_socket, temporary_buffer, BUFFER_SIZE, 0)), "recv error");
			memcpy(recv_buffer + bytes_read, temporary_buffer, (size_t)temp_bytes_read);

			bytes_read += temp_bytes_read;
			recv_buffer[bytes_read] = '\0';
		}

		
		int error = 0;
		struct http_request_header* http_header = http_request_header_parse((char*)recv_buffer, &error);

		free(temporary_buffer);
		free(recv_buffer);

		if (http_header == NULL) {
			if (error == 403) { http_response_send_forbidden(client_socket);}
			else if (error == 500) { fprintf(stderr, "%s\n", http_code_string[INTERNAL_SERVER_ERROR]); exit(EXIT_FAILURE);}
			else { http_response_send_not_implemented(client_socket); break;}
		} else {

			char filename[2000];
			char* host_name;
			
			if ((host_name = http_request_get_host(http_header)) == NULL) {
				http_response_send_not_implemented(client_socket);
				break;
			}
			
			sprintf(filename, "./%s/%s/%s", catalog_name, host_name, http_header->url);

			struct stat st;

			if (access(filename, F_OK) == 0) {
				check(stat(filename, &st), "stat error");

				if (S_ISREG(st.st_mode)) http_response_send_file(&st, http_header, filename, client_socket);
				else http_response_send_redirect(http_header, client_socket);
			} else http_response_send_not_found(client_socket);

			if (http_request_content_equal(http_header, "Connection", "close")) break;

			http_request_header_destroy(http_header);
			free(http_header);
		}
	}

	check(close(client_socket), "close error");

	return NULL;
}

int main(int argc, char** argv) {
	
	if (argc != 3) {
		printf("USAGE: %s <port> <catalog_name>\n", argv[0]);
		return EXIT_SUCCESS;
	}

	pthread_attr_t detached; 

	pthread_attr_init(&detached);
    pthread_attr_setdetachstate(&detached, PTHREAD_CREATE_DETACHED);

	u_int16_t port = (u_int16_t)atoi(argv[1]);
	size_t catalog_name_size = strlen(argv[2]);
	catalog_name = (char*)malloc(catalog_name_size * sizeof(char));
	if (catalog_name == NULL) ERROR("Can not allocate catalog name string!");

	strncpy(catalog_name, argv[2], catalog_name_size);

	int server_socket = init_server(port, 64);

	for (;;) {

		int* client_socket = (int*)malloc(sizeof(int));
		if (client_socket == NULL) ERROR("malloc error");

		*client_socket = accept_connection(server_socket);

		pthread_t thread;
		int error = pthread_create(&thread, &detached, &handle_connection, (void*)client_socket);
		if (error) ERROR("Failed to create a thread!");

		client_socket = NULL;
	}

	free(catalog_name);
	pthread_attr_destroy(&detached);

	return EXIT_SUCCESS;
}