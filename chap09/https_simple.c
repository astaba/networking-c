/* https_simple.c */

#include "chap09.h"

int main(int argc, char *argv[]) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    return EXIT_FAILURE;
  }
#endif

  // Initialize OpenSSL library >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  SSL_library_init();
  OpenSSL_add_all_algorithms();
  SSL_load_error_strings();

  // Create SSL context
  SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
  if (!ctx) {
    fprintf(stderr, "SSL_CTX_new() failed.\n");
    return EXIT_FAILURE;
  }

  // INFO: If we were going to do certificate verification, this would be a good
  // place to include the SSL_CTX_load_verify_locations() function call, as
  // explained in the Certificates section of this chapter. We're omitting
  // certification verification in this example for simplicity, but it is
  // important to include it in real-world applications.
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  // Check that hostname and port were passed
  if (argc < 3) {
    fprintf(stderr, "usage: https_simple hostname port\n");
    return EXIT_FAILURE;
  }
  // NOTE: Standard HTTPS port is 443 but the program let the user specify any.
  char *hostname = argv[1];
  char *port = argv[2];

  // Configure remote address for connection
  printf("Configure remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo *peer_address;
  if (getaddrinfo(hostname, port, &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Remote address is: ");
  char address_buf[100];
  char service_buf[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buf,
              sizeof(address_buf), service_buf, sizeof(service_buf),
              NI_NUMERICHOST);
  printf("%s %s\n", address_buf, service_buf);

  // Create socket
  printf("Creating socket...\n");
  SOCKET server;
  server = socket(peer_address->ai_family, peer_address->ai_socktype,
                  peer_address->ai_protocol);
  if (!ISVALIDSOCKET(server)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Connecting...\n");
  if (connect(server, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(peer_address);

  printf("Connected.\n\n");
  // At this point, a TCP connection has been established. If we didn't need
  // encryption, we could communicate over it directly. However, we are going to
  // use OpenSSL to initiate a TLS/SSL connection over our TCP connection.

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Create a new SSL object, set the hostname for SNI, and initiate the
  // TLS/SSL handshake:
  SSL *ssl = SSL_new(ctx);
  if (!ssl) {
    fprintf(stderr, "SSL_new() failed.\n");
    exit(EXIT_FAILURE);
  }
  if (!SSL_set_tlsext_host_name(ssl, hostname)) {
    fprintf(stderr, "SSL_set_tlsext_host_name() failed.\n");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
  SSL_set_fd(ssl, server);
  if (SSL_connect(ssl) == -1) {
    fprintf(stderr, "SSL_connect() failed.\n");
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
  // The preceding is explained in "Encrypted Sockets with OpenSSL" section.

  printf("SSL/TLS cipher: %s\n", SSL_get_cipher(ssl));

  X509 *cert = SSL_get_peer_certificate(ssl);
  if (!cert) {
    fprintf(stderr, "SSL_get_peer_certificate() failed.\n");
    exit(EXIT_FAILURE);
  }
  char *tmp;
  if ((tmp = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0))) {
    printf("subject: %s\n", tmp);
    OPENSSL_free(tmp);
  }
  if ((tmp = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0))) {
    printf("issuer: %s\n", tmp);
    OPENSSL_free(tmp);
  }

  X509_free(cert);

  // Send HTTPS request
  char buffer[2048];

  snprintf(buffer, sizeof(buffer), "GET / HTTP/1.1\r\n");
  snprintf(buffer + strlen(buffer), sizeof(buffer), "Host: %s:%s\r\n", hostname,
           port);
  snprintf(buffer + strlen(buffer), sizeof(buffer), "Connection: close\r\n");
  snprintf(buffer + strlen(buffer), sizeof(buffer),
           "User-agent: HONPWC http_simple 1.0\r\n");
  snprintf(buffer + strlen(buffer), sizeof(buffer), "\r\n");

  SSL_write(ssl, buffer, strlen(buffer));
  printf("\nSent Headers:\n%s", buffer);

  // Wait for data from the server
  while (1) {
    int bytes_received = SSL_read(ssl, buffer, sizeof(buffer));
    if (bytes_received < 1) {
      printf("\nConnection closed by peer.\n");
      break;
    }

    printf("Received (%d bytes): '%.*s'\n", bytes_received, bytes_received,
           buffer);
  }

  // Shutdown and cleanup
  printf("\nClosing socket...\n");
  SSL_shutdown(ssl);
  CLOSESOCKET(server);
  SSL_free(ssl);
  SSL_CTX_free(ctx);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
