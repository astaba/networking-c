// ch04-establishing-udp-connections/udp_sendto.c
/**
 * @file udp_sendto.c
 * @brief A simple UDP client that sends data to a server.
 * Configures a remote address and port for the server, creates a socket,
 * sends a message to the server, and then closes the socket.
 */

#include "chap04.h"

int main(int argc, char *argv[]) {
#if defined(_WIN32)
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsa_error = WSAStartup(wVersionRequested, &WSAData);
  if (wsa_error) {
    REPORT_SOCKET_ERROR("WSAStartup() failed");
    exit(EXIT_FAILURE);
  }
#endif

  if (argc < 3) {
    fprintf(stderr, "usage: udp_sendto <hostname> <port>\n");
    exit(EXIT_FAILURE);
  }

  printf("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  // INFO: We did not set AF_INET(6) allowing getaddrinfo
  // to return the appropriate address family
  struct addrinfo *peer_address;
  // TEST: hostname = localhost, port = 8080
  int gai_err = getaddrinfo(argv[1], argv[2], &hints, &peer_address);
  if (gai_err) {
    fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(gai_err));
    exit(EXIT_FAILURE);
  }

  // Optionally print configured remote address
  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  int gni_err =
      getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
                  address_buffer, sizeof(address_buffer), service_buffer,
                  sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
  if (gni_err) {
    fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(gni_err));
    exit(EXIT_FAILURE);
  }
  printf("%s %s\n", address_buffer, service_buffer);

  // After successful DNS resolution
  printf("Creating socket...\n");
  SOCKET socket_peer =
      socket(peer_address->ai_family, peer_address->ai_socktype,
             peer_address->ai_protocol);
  if (BAD_SOCKET(socket_peer)) {
    REPORT_SOCKET_ERROR("socket() failed");
    exit(EXIT_FAILURE);
  }

  const char *message = "Hello world";
  printf("Sending: '%s'\n", message);
  int bytes_sent = sendto(socket_peer, message, strlen(message), 0,
                          peer_address->ai_addr, peer_address->ai_addrlen);
  printf("Sent %d bytes.\n", bytes_sent);

  freeaddrinfo(peer_address);
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif
  printf("Finished.\n");
  return EXIT_SUCCESS;
}
