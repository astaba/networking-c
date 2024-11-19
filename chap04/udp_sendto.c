/**
 * @file udp_sendto.c
 * @brief A simple UDP client that sends data to a server.
 */

#include "../mylib/omniplat.h"
#include <stdlib.h>

/**
 * @brief Main function to set up and run the UDP client.
 * 
 * Configures a remote address and port for the server, creates a socket,
 * sends a message to the server, and then closes the socket.
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

  // Configure remote address
  printf("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *peer_address;
  if (getaddrinfo("127.0.0.1", "8080", &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed with error %d\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  // Print configured remote address
  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST | NI_NUMERICSERV);
  printf("%s %s\n", address_buffer, service_buffer);

  // Create socket
  printf("Creating socket...\n");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() failed with error %d", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  // Send data directely to remote server with sendto(). No nedd for connect()
  const char *message = "Hello, World!";
  printf("Sending: %s\n", message);
  int bytes_sent = sendto(socket_peer, message, strlen(message), 0,
                          peer_address->ai_addr, peer_address->ai_addrlen);
  printf("Sent %d bytes.\n", bytes_sent);

  // Close communication routines
  freeaddrinfo(peer_address);
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
