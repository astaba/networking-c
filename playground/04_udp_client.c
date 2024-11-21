/* 04_udp_client.c */

// clang-format off
#include "../chap03/chap03.h"
#if defined(_WIN32)
  #include <conio.h>
#endif
// clang-format on

int main(int argc, char *argv[]) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  if (argc < 3) {
    fprintf(stderr, "usage: tcp_client hostname port\n");
    exit(EXIT_FAILURE);
  }

  printf("Configure remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *peer_address;
  if (getaddrinfo(argv[1], argv[2], &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  // Optionally display remote address as a good debugging practice
  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST);
  printf("%s %s\n", address_buffer, service_buffer);

  printf("Create peer socket...\n");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  // Since this is a UPD connection connect() do not establish any connection
  // (thus no hanshake verification) with the address it is passed. In the the
  // UDP connection, connect() just binds the socket to the address.
  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(peer_address);

  printf("To send data, enter text followed by <Enter>.\n");

  while (1) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_peer, &readfds);
#if !defined(_WIN32)
    FD_SET(fileno(stdin), &readfds);
#endif

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    // timeout.tv_usec = 0;

    /* if (select(socket_peer + 1, &readfds, 0, 0, 0) < 0) { */
    if (select(socket_peer + 1, &readfds, 0, 0, &timeout) < 0) {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(socket_peer, &readfds)) {
      char read_buf[4096];
      int bytes_received = recv(socket_peer, read_buf, sizeof(read_buf), 0);
      if (bytes_received < 1) {
        printf("Connection closed by peer.\n");
        break;
      }

      printf("Received (%d bytes): '%.*s'\n", bytes_received, bytes_received,
             read_buf);
    }

#if defined(_WIN32)
    if (_kbhit()) {
#else
    if (FD_ISSET(0, &readfds)) {
#endif
      char read_buf[4096];
      if (fgets(read_buf, sizeof(read_buf), stdin) == NULL)
        break;
      read_buf[strcspn(read_buf, "\n")] = '\0';
      int bytes_sent = send(socket_peer, read_buf, strlen(read_buf), 0);
      printf("Sent (%d bytes): %.*s\n", bytes_sent, bytes_sent, read_buf);
    }
  }

  printf("Closing socket...\n");
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
