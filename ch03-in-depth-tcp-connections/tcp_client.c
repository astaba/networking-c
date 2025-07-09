// ch03-in-depth-tcp-connections/tcp_client.c

#include "chap03.h"
/**
 * @brief Entry point of the TCP client application.
 *
 * This function establishes a TCP connection to the specified remote host and
 * port. It uses the `select()` function for I/O multiplexing, allowing the
 * program to handle both sending and receiving data synchronously from the
 * command line and through the peer socket. The function will continuously
 * prompt the user for input to send to the connected server and will display
 * any received messages from the server.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments, where argv[1] is the hostname
 *             and argv[2] is the port.
 * @return Returns EXIT_SUCCESS on successful execution, or exits with an
 *         error message on failure.
 */
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
    fprintf(stderr, "usage: tcp_client hostname port\n");
    exit(EXIT_FAILURE);
  }

  printf("Configure remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo *peer_address;
  // INFO: We check the return value of getaddrinfo() because we're resolving a
  // remote address, typically using DNS or local hostname configuration. If
  // this lookup fails, the client cannot proceed to connect. Unlike passive
  // local resolution for binding (e.g., with NULL host), this actively depends
  // on external or system-level resolution that may fail due to invalid input,
  // unreachable DNS, or misconfigured system settings.
  int gai_err = getaddrinfo(argv[1], argv[2], &hints, &peer_address);
  if (gai_err) {
    fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(gai_err));
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
  if (BAD_SOCKET(socket_peer)) {
    REPORT_SOCKET_ERROR("socket() failed");
    exit(EXIT_FAILURE);
  }

  // INFO: Like bind(), connect() associates a socket with a specific address.
  // Furthermore connect() also initiates a TCP handshake with the remote host,
  // ensuring client side establishes a stateful connection.
  printf("Connection...\n");
  if (connect(socket_peer, peer_address->ai_addr, peer_address->ai_addrlen)) {
    REPORT_SOCKET_ERROR("connect() failed");
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(peer_address);

  printf("Connected.\n");
  printf("To send data, enter text followed by <Enter>.\n");

  while (1) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_peer, &readfds);
#if !defined(_WIN32)
    FD_SET(fileno(stdin), &readfds);
#endif
    // INFO: On non-Windows system select() can be used to monitor for terminal
    // input on stdin == 0. Therefore we must unlock the blocking select() by
    // passing a timeout to go and check for terminal input on the Windows
    // _kbhit() on each iteration.
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    if (select(socket_peer + 1, &readfds, 0, 0, &timeout) < 0) {
      REPORT_SOCKET_ERROR("select() failed");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(socket_peer, &readfds)) {
      char read_buf[4096];
      int bytes_recv = recv(socket_peer, read_buf, sizeof(read_buf), 0);
      if (bytes_recv < 1) {
        printf("Connection closed by peer.\n");
        break;
      }

      printf("Received (%d byted):\n%.*s\n", bytes_recv, bytes_recv, read_buf);
    }

#if defined(_WIN32)
    if (_kbhit()) {
#else
    if (FD_ISSET(0, &readfds)) {
#endif
      char read_buf[4096];
      // To match this if{} condition on linux:
      // press <C-D> without preceding input
      if (fgets(read_buf, sizeof(read_buf), stdin) == NULL) {
        printf("Connection terminated for input error.\n");
        break;
      }
      int bytes_sent = send(socket_peer, read_buf, strlen(read_buf), 0);
      printf("Sent (%d bytes): %s\n", bytes_sent, read_buf);
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
