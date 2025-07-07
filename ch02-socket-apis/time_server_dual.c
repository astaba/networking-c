// ch02-socket-apis/time_server_dual.c

#include "chap02.h"
/**
 * @brief Cross-platform TCP server using IPv6 with dual-stack support.
 *
 * This minimal HTTP server listens on port 8080 and accepts a single client
 * connection. The server demonstrates how to configure an IPv6 socket
 * to operate in dual-stack mode—accepting both IPv6 and IPv4 clients—
 * by explicitly clearing the `IPV6_V6ONLY` option via `setsockopt()`.
 *
 * It uses `getaddrinfo()` for address setup and wraps platform-specific
 * socket handling using macros for compatibility between Windows and Unix-like
 * systems.
 *
 * Once connected, it reads the request, prints the source IP, and responds
 * with the system's local time over a valid HTTP response.
 */
int main(void) {
#ifdef _WIN32
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsa_error = WSAStartup(wVersionRequested, &WSAData);
  if (wsa_error) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  printf("Configuring local address...\n");
  struct addrinfo hints;            // Hints for address configuration
  memset(&hints, 0, sizeof(hints)); // Zero out the structure
  hints.ai_family = AF_INET6;       // IPv6 family
  hints.ai_socktype = SOCK_STREAM;  // Stream socket (TCP)
  hints.ai_flags = AI_PASSIVE;      // Bind to any available address
  // INFO: getaddrinfo generated an address suitable for later bind()
  // Only privilege users can bind to port 0 through 1023
  // Ports are reserved to only one program. If bind() fails change port nbr
  struct addrinfo *bind_address;
  getaddrinfo(0, "8080", &hints, &bind_address);

  printf("Creating socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (BAD_SOCKET(socket_listen)) {
    REPORT_SOCKET_ERROR("socket() failed");
    return EXIT_FAILURE;
  }

  // INFO: Make the server dual stack.
  // The socket must initially created as IPv6 then after socket() and before
  // bind() call setsockopt() to target the IPPROTO_IPV6 part of the socket
  // and clear the IPV6_V6ONLY option.
  int option = 0;
  if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&option,
                 sizeof(option))) {
    REPORT_SOCKET_ERROR("setsockopt() failed");
    return EXIT_FAILURE;
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    REPORT_SOCKET_ERROR("bind() failed");
    return EXIT_FAILURE;
  }
  freeaddrinfo(bind_address); // Free the dynamically allocated struct

  printf("Listening...\n");
  // INFO: Start listening for incoming connections
  // Queue up 10 conections at most before rejecting incoming
  if (listen(socket_listen, 10) < 0) {
    REPORT_SOCKET_ERROR("listen() failed");
    return EXIT_FAILURE;
  }

  printf("Waiting for connection...\n");
  // Structure to hold the client's address
  struct sockaddr_storage client_address;
  socklen_t client_len = sizeof(client_address);
  // Blocking call to accept an incoming connection
  SOCKET socket_client =
      accept(socket_listen, (struct sockaddr *)&client_address, &client_len);
  if (BAD_SOCKET(socket_client)) {
    REPORT_SOCKET_ERROR("accept() failed");
    return EXIT_FAILURE;
  }

  // Optionally log client data
  printf("\nClient is connected...\n");
  char address_buffer[100];
  getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer,
              sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
  printf("%s\n", address_buffer);

  printf("Reading request...\n");
  char request[1024];
  // WARN: Blocking call: In production you should always check: recv() > 0
  int bytes_received = recv(socket_client, request, 1024, 0);
  printf("Received %d bytes.\n", bytes_received);
  printf("%.*s", bytes_received, request);

  // Send response headers
  printf("Sending response...\n");
  const char *res_header = "HTTP/1.1 200 OK\r\n"
                           "Connection: close\r\n"
                           "Content-Type: text/plain\r\n\r\n"
                           "Local time is: ";
  size_t bytes_sent = send(socket_client, res_header, strlen(res_header), 0);
  printf("Sent %zu of %d bytes.\n", bytes_sent, (int)strlen(res_header));

  // Send response body
  time_t now = time(NULL);
  const char *time_str = ctime(&now);
  bytes_sent = send(socket_client, time_str, strlen(time_str), 0);
  printf("Sent %zu of %d bytes.\n", bytes_sent, (int)strlen(time_str));

  // Close client's socket to prevent browser from waiting until it times out
  printf("Closing connection...\n");
  CLOSESOCKET(socket_client);
  // NOTE: At this point a production server would annouce:
  // printf("Waiting for next connection...\n");
  // accept additional connections by calling accept() once again

  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);

#ifdef _WIN32
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
