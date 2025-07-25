/* 04_udp_recvfrom.c */

#include "../chap04/chap04.h"

/* UPD server to receive only one packet and shutdown */
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

  printf("Create socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
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

  // Without connecting in any sort, just receive data from any UPD client.
  char read_buf[1024];
  int bytes_received;
  struct sockaddr_storage client_address;
  socklen_t client_address_len = sizeof(client_address);
  bytes_received =
      recvfrom(socket_listen, read_buf, sizeof(read_buf), 0,
               (struct sockaddr *)&client_address, &client_address_len);

  // Print the address of client
  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo((struct sockaddr *)&client_address, client_address_len,
              address_buffer, sizeof(address_buffer), service_buffer,
              sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
  printf("%s:%s\n", address_buffer, service_buffer);

  // Printf the received data.
  printf("Received (%d bytes):\n'%.*s'\n", bytes_received, bytes_received,
         read_buf);

  // Shutdown
  CLOSESOCKET(socket_listen);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("\nFinished.\n");
  return EXIT_SUCCESS;
}
