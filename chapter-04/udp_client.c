/* udp_client.c */

#include "../mylib/omniplat.h"
#include <stdlib.h>

// Required on Windows for _kbhit() to indicate terminal input is waiting
#if defined(_WIN32)
#include <conio.h>
#endif

int main(int argc, char *argv[]) {

  // Initialize Winsock
#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return EXIT_FAILURE;
  }
#endif

  // Check enough arguments from cmd-line (after program name)
  if (argc < 3) {
    fprintf(stderr, "usage: tcp_client hostname port\n");
    return EXIT_FAILURE;
  }

  // Configure remote address for connection
  printf("Configuring a remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  // INFO: Only difference between UDP and TCP clients
  hints.ai_socktype = SOCK_DGRAM;
  struct addrinfo *peer_address;
  if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  // Print remote address as an optional good debugging measure
  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST);
  printf("%s %s\n", address_buffer, service_buffer);

  // We can then create our socket
  printf("Create socket...\n");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  // Establish connection to remote server by calling connect()
  printf("Connecting...\n");
  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }
  // At this point a TCP connection is established with the remote server.
  // Therefore we can free memory for peer_address
  freeaddrinfo(peer_address);

  // Let the user know by printing message and instructions about sending data.
  printf("Connected.\n");
  printf("To send data, enter text followed by enter.\n");

  // Now we need to loop while checking both terminal and socket for new data.
  while (1) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(socket_peer, &reads);
#if !defined(_WIN32)
    // On non-windows system select() monitors terminal input aswell
    // O is stdin file descriptor also got by: fileno(stdin)
    FD_SET(0, &reads);
#endif

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    if (select(socket_peer + 1, &reads, 0, 0, &timeout) < 0) {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      return EXIT_FAILURE;
    }

    if (FD_ISSET(socket_peer, &reads)) {
      char read[4096];
      int bytes_received = recv(socket_peer, read, 4096, 0);
      if (bytes_received < 1) {
        printf("Connection closed by peer.\n");
        break;
      }
      printf("\nReceived (%d bytes): %.*s", bytes_received, bytes_received, read);
    }

#if defined(_WIN32)
    if (_kbhit()) {
#else
    if (FD_ISSET(0, &reads)) {
#endif
      char read[4096];
      if (!fgets(read, 4096, stdin))
        break;
      printf("Sending: %s", read);
      int bytes_sent = send(socket_peer, read, strlen(read), 0);
      printf("Sent %d bytes.\n", bytes_sent);
    }

  } // end while

  printf("Closing socket...\n");
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}

