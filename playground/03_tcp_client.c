/* 03_tcp_client.c */

// clang-format off
#include "../chap03/chap03.h"
#include <stdio.h>
#if defined (_WIN32)
  #include <conio.h>
#endif
// clang-format on

/**
 * @brief Entry point of the TCP client application.
 *
 * This function establishes a TCP connection to the specified remote host and
 * port. It uses the `select()` function for I/O synchronous multiplexing,
 * allowing the program to handle both sending and receiving data from the
 * command line and through the peer socket. The function will continuously
 * prompt the user for input to send to the connected server and will display
 * any received messages from the server.
 * WARN: Keep in mind that in a production TCP client you would never need to
 * monitor the stdin for input.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments, where argv[1] is the hostname
 *             and argv[2] is the port.
 * @return Returns EXIT_SUCCESS on successful execution, or exits with an
 *         error message on failure.
 */
int main(int argc, char *argv[]) {

  if (argc < 3) {
    fprintf(stderr, "Usage:\ttcp_client hostname port\n");
    exit(EXIT_FAILURE);
  }

  char *hostname = argv[1];
  char *port = argv[2];

#if defined(_WIN32)
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WSAStartup(wVersionRequested, &WSAData);
  if (!wsaerr) {
    printf("The dll found.\n");
    printf("Winsock Status: %s\n", WSAData.szSystemStatus);
  } else {
    fprintf(stderr, "Failed to initialize Winsock. The dll not found.\n");
    exit(EXIT_FAILURE);
  }
#endif

  printf("Configure remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *peer_address;
  if (getaddrinfo(hostname, port, &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed with error %d\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST);
  printf("%s %s\n", address_buffer, service_buffer);

  printf("Create socket...\n");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() failed with error (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Connecting to remote server...\n");
  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() failed with error (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(peer_address);
  printf("Connected.\n");
  printf("Input data on the prompt and press <Enter> to send.\n\n");

  fd_set masterfds;
  FD_ZERO(&masterfds);
  FD_SET(socket_peer, &masterfds);
#if !defined(_WIN32)
  FD_SET(fileno(stdin), &masterfds);
#endif

  while (1) {

    fd_set readfds = masterfds;

#if defined(_WIN32)
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    if (select(socket_peer + 1, &readfds, 0, 0, &timeout) < 0) {
#else
    if (select(socket_peer + 1, &readfds, 0, 0, 0) < 0) {
#endif
      fprintf(stderr, "select() failed with error (%d)\n", GETSOCKETERRNO());
      exit(EXIT_FAILURE);
    };

    if (FD_ISSET(socket_peer, &readfds)) {
      char recv_buf[4096];
      int b8_rcvd = recv(socket_peer, recv_buf, 4096, 0);
      if (b8_rcvd < 1) {
        printf("Connection closed by peer.\n");
        break;
      }
      printf("Received data (%d bytes): '%.*s'\n\n", b8_rcvd, b8_rcvd,
             recv_buf);
    }

#if defined(_WIN32)
    if (_kbhit()) {
#else
    if (FD_ISSET(0, &readfds)) {
#endif

      char send_buf[4096];
      if (fgets(send_buf, 4096, stdin) != NULL) {
        send_buf[strcspn(send_buf, "\n")] = '\0';
        if (strlen(send_buf) == 0)
          break;
      } else {
        break;
      }
      printf("Sending: ");
      int b8_sent = send(socket_peer, send_buf, strlen(send_buf), 0);
      printf(" %d bytes of %zu\n", b8_sent, strlen(send_buf));
    }
  }

  printf("Closing connection...\n");
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
