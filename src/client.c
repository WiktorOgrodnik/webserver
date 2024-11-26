#include "http_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <gnutls/gnutls.h>
#include <arpa/inet.h>

#define SERVER_PORT 2024
#define SERVER_HOST "localhost"
#define BUFFER_SIZE 65536
#define CA_CERT_FILE "myCA.pem"

gnutls_certificate_credentials_t x509_cred;

static int client_init(u_int16_t port) {
	int client_socket;
	struct sockaddr_in server_addr;

	check((client_socket = socket(AF_INET, SOCK_STREAM, 0)), "socket error");

	server_addr = (struct sockaddr_in){ .sin_family = AF_INET, .sin_port = htons(port)};

  check(inet_pton(AF_INET, SERVER_HOST, &server_addr.sin_addr), "inet_pton error");
  check(connect(client_socket, &server_addr, sizeof(server_addr)), "connect error");

	return client_socket;
}

static void tls_init(gnutls_session_t* session, int client_socket) {
  int ret;
  gnutls_certificate_allocate_credentials(&x509_cred);

  if ((ret = gnutls_certificate_set_x509_trust_file(x509_cred, CA_CERT_FILE, GNUTLS_X509_FMT_PEM)) < 0) {
    fprintf(stderr, "failed to load CA_CERT_FILE CA: %s\n", gnutls_strerror(ret));
    exit(EXIT_FAILURE);
  }

  gnutls_init(session, GNUTLS_CLIENT);
  gnutls_credentials_set(*session, GNUTLS_CRD_CERTIFICATE, x509_cred);
  gnutls_set_default_priority(*session);
	gnutls_session_set_verify_cert(*session, SERVER_HOST, 0);

  gnutls_transport_set_int(*session, client_socket);
  gnutls_handshake_set_timeout(*session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
}

int main() {
  int client_socket, failed = 0, ret, type;
  unsigned status;
  gnutls_datum_t out;
  gnutls_session_t session;

  char buffer[BUFFER_SIZE];

  client_socket = client_init(SERVER_PORT);
  tls_init(&session, client_socket);

	do {
		ret = gnutls_handshake(session);
	} while (ret < 0 && gnutls_error_is_fatal(ret) == 0);
	if (ret < 0) {
		if (ret == GNUTLS_E_CERTIFICATE_VERIFICATION_ERROR) {
			/* check certificate verification status */
			type = gnutls_certificate_type_get(session);
			status = gnutls_session_get_verify_cert_status(session);
			check(gnutls_certificate_verification_status_print(status, type, &out, 0), "gnutls_certificate_verification_status_print failed");
			printf("cert verify output: %s\n", out.data);
			gnutls_free(out.data);
		}
		fprintf(stderr, "*** Handshake failed: %s\n", gnutls_strerror(ret));
    failed = 1;
		goto tls_handshake_failed;
	} else {
		char* desc = gnutls_session_get_desc(session);
		printf("- Session info: %s\n", desc);
		gnutls_free(desc);
	}

  for (;;) {
    char* e = fgets(buffer, BUFFER_SIZE, stdin);
    if (e == NULL) {
      fprintf(stderr, "fgets failed\n");
      failed = true;
      break;
    }

    size_t len = strlen(buffer);
    if (len == 0)
      break;

    if (buffer[len - 1] == '\n')
      buffer[len - 1] = '\0';
    strcat(buffer, "\r\n");

    ret = gnutls_record_send(session, buffer, strlen(buffer));
    if (ret < 0) {
      fprintf(stderr, "gnutls_record_send failed: %s\n", gnutls_strerror(ret));
      failed = true;
      break;
    }

    memset(buffer, 0, BUFFER_SIZE);
    ret = gnutls_record_recv(session, buffer, BUFFER_SIZE - 1);
    if (ret < 0) {
      fprintf(stderr, "gnutls_record_recv failed: %s\n", gnutls_strerror(ret));
      failed = true;
      break;
    }

    printf("%s\n", buffer);
  }

  gnutls_bye(session, GNUTLS_SHUT_RDWR);
tls_handshake_failed:
  gnutls_deinit(session);
  close(client_socket);
  gnutls_certificate_free_credentials(x509_cred);
  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
