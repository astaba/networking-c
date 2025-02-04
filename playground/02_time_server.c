/* 02_time_server.c */

#include "../chap02/chap02.h"
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>

int main(void) {

#if defined(_WIN32)
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WASStartup(wVersionRequested, &WASStartup);
  if (wsaerr) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  printf("Configuring server local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  /* hints.ai_family = AF_INET6; */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;

  struct addrinfo *bind_address;
  getaddrinfo(0, "8080", &hints, &bind_address);

  printf("Creating socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  // Make the socket dual stack
  int option = 0;
  if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&option,
                 sizeof(option))) {
    fprintf(stderr, "setsockopt() failed. (%d)", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Listening... ");
  if (listen(socket_listen, 10) < 0) {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  printf("on port 8080\n");

  printf("Waiting for connection...\n");
  struct sockaddr_storage client_address;
  socklen_t client_address_len = sizeof(client_address);
  SOCKET socket_client = accept(
      socket_listen, (struct sockaddr *)&client_address, &client_address_len);
  if (!ISVALIDSOCKET(socket_client)) {
    fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Client is connected: ");
  char address_buffer[100];
  getnameinfo((struct sockaddr *)&client_address, client_address_len,
              address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
  printf("%s\n", address_buffer);

  printf("\nReading request...\n");
  char recv_buf[2048];
  int b8_rcvd = recv(socket_client, recv_buf, 2048, 0);
  printf("Received data (%d bytes)\n'%.*s'\n", b8_rcvd, b8_rcvd, recv_buf);

  printf("\nSending response...\n");
  time_t timer;
  time(&timer);
  char time_msg[100];
  snprintf(time_msg, 100, "Local time is: %s\n", ctime(&timer));
  size_t content_length = strlen(time_msg);

  char resp_buf[1024];
  snprintf(resp_buf, 1024,
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: %zu\r\n"
           "Connection: close\r\n"
           "\r\n"
           "%s",
           content_length, time_msg);

  int b8_sent = send(socket_client, resp_buf, strlen(resp_buf), 0);
  printf("Sent %d bytes of %zu\n", b8_sent, strlen(resp_buf));

  printf("Closing connection...\n");
  CLOSESOCKET(socket_client);

  // In production the server should start looping to wait for new connections.

  printf("\nClosing listening socket...\n");
  CLOSESOCKET(socket_listen);

#if defined(_WIN32)
  WASCleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
