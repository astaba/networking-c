/**
 * @file udp_recvfrom.c
 * @brief A simple UDP server that receives data from clients.
 */

#include "../mylib/omniplat.h"
#include <stdlib.h>

/**
 * @brief Main function to set up and run the UDP server.
 *
 * Configures a local address and port for the server, binds a socket to it,
 * receives data from a client, prints the received data and sender's
 * information, and then closes the socket.
 *
 * @return EXIT_SUCCESS on successful completion, EXIT_FAILURE otherwise.
 */
int main(void) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return EXIT_FAILURE;
  }
#endif

  // Configure local address the server is listening on.
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(0, "8080", &hints, &bind_address);

  // Create socket
  printf("Create socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed with error %d\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  // Bind socket to local address
  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed with erro %d\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }
  freeaddrinfo(bind_address);

  // INFO: From here onward code is specific to UDP socket
  // Once the local address is bound we simply start to receivee data. There is
  // no need to call listen() or accept().
  struct sockaddr_storage client_address;
  socklen_t client_len = sizeof(client_address);
  char read[1024];
  int bytes_received =
      recvfrom(socket_listen, read, 1024, 0, (struct sockaddr *)&client_address,
               &client_len);

  // Print the received data
  printf("Received (%d bytes): %.*s\n", bytes_received, bytes_received, read);

  // Usefuf option: print sender's address and port number.
  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(((struct sockaddr *)&client_address), client_len, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST | NI_NUMERICSERV);

  // Close socket
  CLOSESOCKET(socket_listen);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");

  return EXIT_SUCCESS;
}
