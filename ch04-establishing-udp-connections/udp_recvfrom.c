// ch04-establishing-udp-connections/udp_recvfrom.c
/**
 * @file udp_recvfrom.c
 * @brief A simple UDP server that receives data from clients.
 * Configures a local address and port for the server, binds a socket to it,
 * receives data from a client, prints the received data and sender's
 * information, and then closes the socket.
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

  printf("Configure local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(0, "8080", &hints, &bind_address);

  printf("Create socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
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

  // INFO: From here onward code is specific to UDP socket.
  // Once the local address is bound we simply start to receivee data.
  // There is no need to call listen() or accept().
  struct sockaddr_storage client_address;
  socklen_t client_len = sizeof(client_address);
  char read_buf[1024];
  // recvfrom() is like a combination of TCP accept() and recv()
  int bytes_recv = recvfrom(socket_listen, read_buf, 1024, 0,
                            (struct sockaddr *)&client_address, &client_len);

  // Print the received data
  printf("Received (%d bytes): %.*s\n", bytes_recv, bytes_recv, read_buf);

  // Usefuf option: print sender's address and port number.
  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST | NI_NUMERICSERV);
  printf("%s %s\n", address_buffer, service_buffer);

  // Close socket
  CLOSESOCKET(socket_listen);

#if defined(_WIN32)
  WSACleanup();
#endif
  printf("Finished.\n");
  return EXIT_SUCCESS;
}
