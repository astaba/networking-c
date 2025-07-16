// ch04-establishing-udp-connections/udp_client.c

#include "chap04.h"

int main(int argc, char *argv[]) {
#if defined(_WIN32)
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsa_error = WSAStartup(wVersionRequested, &WSAData);
  if (wsa_error) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  if (argc < 3) {
    fprintf(stderr, "usage: udp_client hostname port\n");
    exit(EXIT_FAILURE);
  }

  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *peer_address;
  int gai_err = getaddrinfo(argv[1], argv[2], &hints, &peer_address);
  if (gai_err) {
    fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(gai_err));
    exit(EXIT_FAILURE);
  }

  // Optionally print the result of getaddrinfo DNS resolution
  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST);
  printf("%s %s\n", address_buffer, service_buffer);

  printf("Create peer socket...");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (BAD_SOCKET(socket_peer)) {
    REPORT_SOCKET_ERROR("socket() failed");
    exit(EXIT_FAILURE);
  }

  // INFO: Although connect() still associates local address in UPD it doesn't
  // establish a connection — no handshake like TCP but it merely sets a
  // default peer address, simplifying send/recv calls.
  printf("Connecting...\n");
  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    REPORT_SOCKET_ERROR("connect() failed");
    exit(EXIT_FAILURE);
  }
  // At this point a UPD connection is established with the remote server.
  // The dynamically allocated struct must be free
  freeaddrinfo(peer_address);

  // Let the user know by printing message and instructions about sending data.
  printf("Connected.\n");
  printf("To send data, enter text followed by enter.\n");

  while (1) {
    fd_set reads;
    FD_ZERO(&reads);
    FD_SET(socket_peer, &reads);
#if !defined(_WIN32)
    FD_SET(fileno(stdin), &reads);
#endif

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    if (select(socket_peer + 1, &reads, NULL, NULL, &timeout) < 0) {
      REPORT_SOCKET_ERROR("select() failed");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(socket_peer, &reads)) {
      char read_buf[4096];
      int bytes_recv = recv(socket_peer, read_buf, 4096, 0);
      if (bytes_recv < 1) {
        printf("Connection closed by peer.\n");
        break;
      }
      printf("Received (%d bytes): %.*s\n", bytes_recv, bytes_recv, read_buf);
    }

#if defined(_WIN32)
    if (_kbhit()) {
#else
    if (FD_ISSET(0, &reads)) {
#endif
      char read_buf[4096];
      // To match this if{} condition on linux:
      // press <C-D> without preceding input
      if (fgets(read_buf, sizeof(read_buf), stdin) == NULL) {
        printf("Connection terminated for input error.\n");
        break;
      }
      printf("Sending: %s", read_buf);
      int bytes_sent = send(socket_peer, read_buf, strlen(read_buf), 0);
      printf("Sent %d bytes.\n", bytes_sent);
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
