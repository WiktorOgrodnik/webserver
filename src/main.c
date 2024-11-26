#include "http_common.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define BUFFER_SIZE 65536 // 65535 + 1
#define CONNECTION_TIMEOUT 1000

static int init_server(u_int16_t port, int backlog) {
	int server_socket;
	struct sockaddr_in server_addr;

	check((server_socket = socket(AF_INET, SOCK_STREAM, 0)), "socket error");

  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    printf("setsockopt(SO_REUSEADDR) failed");

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

static inline void chr_swap(char* a, char* b) {
  *a ^= *b; *b ^= *a; *a ^= *b;
}

static void strrev(char* start, char* end) {
  while (start < end)
    chr_swap(start++, end--);
}

static bool wspak_incomplete_request(u_int8_t* buffer, ssize_t buffer_size) {

  if (buffer_size == 0)
    return true;
  else if (buffer_size > BUFFER_SIZE)
    return false; 

	uintptr_t end_of_wspak = (uintptr_t)strstr((char*)buffer, "\r\n");

	return end_of_wspak == 0; //return true if \r\n not found, the message is incomplete
}

static size_t wspak_request_len(u_int8_t* buffer) {
  uintptr_t start = (uintptr_t)buffer;
	uintptr_t end = (uintptr_t)strstr((char*)buffer, "\r\n");

  return end - start;
}

static void wspak_response_send(u_int8_t* buffer, ssize_t request_size, int client_socket) {
	u_int8_t* reply_buffer = (u_int8_t*)malloc(sizeof(u_int8_t) * (request_size + 2));
	memcpy(reply_buffer, buffer, request_size + 2);

  strrev((char*)reply_buffer, (char*)reply_buffer + request_size);

	check((send(client_socket, reply_buffer, request_size + 2, 0)), "send error");
	free(reply_buffer);
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
		
		while (wspak_incomplete_request(recv_buffer, bytes_read)) {
			fd_set descriptors;
			FD_ZERO(&descriptors);
			FD_SET(client_socket, &descriptors);

			check((ready = select(client_socket + 1, &descriptors, NULL, NULL, &tv)), "select error");
			if (ready == 0)
        break;

			ssize_t temp_bytes_read = 0;
			check((temp_bytes_read = recv(client_socket, temporary_buffer, BUFFER_SIZE, 0)), "recv error");
			memcpy(recv_buffer + bytes_read, temporary_buffer, (size_t)temp_bytes_read);

			bytes_read += temp_bytes_read;
			recv_buffer[bytes_read] = '\0';
		}

		if (bytes_read > BUFFER_SIZE) {
      // error
			fprintf(stderr, "%s\n", "BUFFER overflow");
      exit(EXIT_FAILURE);
		} else {
      size_t request_len = wspak_request_len(recv_buffer);

			if (request_len == 0) 
        goto server_close;
			
			wspak_response_send(recv_buffer, request_len, client_socket);
		}

    free(temporary_buffer);
		free(recv_buffer);
	}
server_close:
	check(close(client_socket), "close error");
	return NULL;
}

int main(int argc, char** argv) {
	
	if (argc != 2) {
		printf("USAGE: %s <port>\n", argv[0]);
		return EXIT_SUCCESS;
	}

	pthread_attr_t detached; 

	pthread_attr_init(&detached);
  pthread_attr_setdetachstate(&detached, PTHREAD_CREATE_DETACHED);

	u_int16_t port = (u_int16_t)atoi(argv[1]);
	int server_socket = init_server(port, 64);

	for (;;) {

		int* client_socket = (int*)malloc(sizeof(int));
		if (client_socket == NULL)
      ERROR("malloc error");

		*client_socket = accept_connection(server_socket);

		pthread_t thread;
		int error = pthread_create(&thread, &detached, &handle_connection, (void*)client_socket);
		if (error)
      ERROR("Failed to create a thread!");

		client_socket = NULL;
	}

	pthread_attr_destroy(&detached);

	return EXIT_SUCCESS;
}
