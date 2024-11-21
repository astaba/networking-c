/* 04_udp_sendto.c */

#include "../chap04/chap04.h"

/* A test UPD client for the test UPD server at upd_recvfrom.c. It just sends
 * one packet and shuts down */
int main(int argc, char *argv[]) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  if (argc < 3) {
    fprintf(stderr, "usage: udp_sendto hostname port\n");
    exit(EXIT_SUCCESS);
  }

  char *hostname = argv[1];
  char *port = argv[2];

  printf("Configure remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *peer_address;
  if (getaddrinfo(hostname, port, &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Remote addres is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST | NI_NUMERICSERV);
  printf("%s:%s\n", address_buffer, service_buffer);

  printf("Create socket...\n");
  SOCKET socket_peer =
      socket(peer_address->ai_family, peer_address->ai_socktype,
             peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Sending data...\n");
  char write_buf[1024];
  snprintf(write_buf, 1024, "Hello from the UPD client.");
  int bytes_sent = sendto(socket_peer, write_buf, strlen(write_buf), 0,
                          peer_address->ai_addr, peer_address->ai_addrlen);
  printf("Sent (%d bytes): '%.*s'\n", bytes_sent, bytes_sent, write_buf);

  freeaddrinfo(peer_address);
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
