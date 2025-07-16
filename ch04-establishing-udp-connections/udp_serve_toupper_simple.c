// ch04-establishing-udp-connections/udp_serve_toupper_simple.c
/**
 * @file udp_serve_toupper_simple.c
 * @brief A simple UDP server to convert lower case to upper case.
 *
 * This program creates a UDP socket, binds it to a local address, and
 * listens for incoming messages. It converts the messages to upper case
 * and sends them back to the sender.
 */

#include "chap04.h"

int main(void) {
#if defined(_WIN32)
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsa_error = WSAStartup(wVersionRequested, &WSAData);
  if (wsa_error) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  printf("Configuring local address...\n");
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
  if (BAD_SOCKET(socket_listen)) {
    REPORT_SOCKET_ERROR("socket() failed");
    exit(EXIT_FAILURE);
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    REPORT_SOCKET_ERROR("bind() failed");
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Waiting for connections...\n");
  while (1) {

    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);

    char read_buf[1024];
    size_t bytes_recv =
        recvfrom(socket_listen, read_buf, sizeof(read_buf), 0,
                 (struct sockaddr *)&client_address, &client_len);
    if (bytes_recv < 1) {
      REPORT_SOCKET_ERROR("Connection closed. ");
      exit(EXIT_FAILURE);
    }

    for (size_t j = 0; j < bytes_recv; j++) {
      read_buf[j] = toupper(read_buf[j]);
    }

    sendto(socket_listen, read_buf, bytes_recv, 0,
           (struct sockaddr *)&client_address, client_len);
  } // while(1)

  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);

#if defined(_WIN32)
  WSACleanup();
#endif
  printf("Finished.\n");
  return EXIT_SUCCESS;
}
