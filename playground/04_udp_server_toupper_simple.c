/* 04_udp_server_toupper_simple.c */

#include "../chap04/chap04.h"
#include <ctype.h>

int main(void) {
#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  printf("Configure local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(0, "8080", &hints, &bind_address);

  printf("Creating socket...\n");
  SOCKET socket_listen =
      socket(bind_address->ai_family, bind_address->ai_socktype,
             bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Waiting for connection...\n");

  while (1) {

    struct sockaddr_storage client_address;
    socklen_t client_address_len = sizeof(client_address);
    char read_buf[1024];
    int bytes_received =
        recvfrom(socket_listen, read_buf, 1024, 0,
                 (struct sockaddr *)&client_address, &client_address_len);
    if (bytes_received < 1) {
      fprintf(stderr, "connection closed. (%d)\n", GETSOCKETERRNO());
      exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < bytes_received; i++) {
      read_buf[i] = toupper(read_buf[i]);
    }

    sendto(socket_listen, read_buf, bytes_received, 0,
           (struct sockaddr *)&client_address, client_address_len);
  }

  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
