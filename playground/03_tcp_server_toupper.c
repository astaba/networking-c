/* 03_tcp_server_toupper.c */

#include "../chap03/chap03.h"
#include <ctype.h>

/* To-uppercase microservice TCP server. It doesn't parse request for headers or
 * body, it just uppercases any text-based bytes received and send it back to
 * the client */
int main(void) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize Winsock.");
    exit(EXIT_FAILURE);
  }
#endif

  printf("Configure local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  char *port = "8080";
  getaddrinfo(0, port, &hints, &bind_address);

  printf("Create listening socket...");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  // Make the socket dual stack
  int option = 0;
  if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&option,
                 sizeof(option))) {
    fprintf(stderr, "setsocketoopt() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Listening...\n");
  if (listen(socket_listen, 10) < 0) {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  printf("Server listnening on port %s.\n", port);

  // Set up fd set for multiplex service
  fd_set masterfds;
  FD_ZERO(&masterfds);
  FD_SET(socket_listen, &masterfds);
  SOCKET max_socket = socket_listen;

  printf("Waiting for connections...\n");

  while (1) {

    fd_set readfds;
    readfds = masterfds;
    if (select(max_socket + 1, &readfds, 0, 0, 0) < 0) {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      exit(EXIT_FAILURE);
    }

    SOCKET i;
    for (i = 1; i <= max_socket; i++) {
      if (FD_ISSET(i, &readfds)) { // Handle sockets
        if (i == socket_listen) {
          struct sockaddr_storage client_address;
          socklen_t client_addr_len = sizeof(client_address);
          SOCKET socket_client =
              accept(socket_listen, (struct sockaddr *)&client_address,
                     &client_addr_len);
          if (!ISVALIDSOCKET(socket_client)) {
            fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
            exit(EXIT_FAILURE);
          }

          FD_SET(socket_client, &masterfds);
          if (socket_client > max_socket)
            max_socket = socket_client;

          printf("New connection from: ");
          char address_buffer[100];
          char service_buffer[100];
          getnameinfo((struct sockaddr *)&client_address, client_addr_len,
                      address_buffer, sizeof(address_buffer), service_buffer,
                      sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
          printf("%s:%s\n", address_buffer, service_buffer);

        } else {
          char read[1024];
          int bytes_received = recv(i, read, 1024, 0);
          if (bytes_received < 1) {
            FD_CLR(i, &masterfds);
            CLOSESOCKET(i);
            continue;
          }
          /* printf("Received (%d bytes): '%.*s'\n", bytes_received,
                 bytes_received, read); */

          int j;
          for (j = 0; j < bytes_received; j++) {
            read[j] = toupper(read[j]);
          }
          read[j] = 0;
          int bytes_sent = send(i, read, bytes_received, 0);
          printf("Sent (%d bytes): %s\n", bytes_sent, read);
        }
      } // if (FD_ISSET(i, &readfds))
    } // for SOCKET i <= max_fd
  } // while(1)

  // INFO: Du to the preceding while loop the next part of the program will
  // never run. Nevertheless, is it a good practice in the event we later add
  // functionality to abort the while loop.

  // Cleanup routines
  printf("Close listening socket...\n");
  CLOSESOCKET(socket_listen);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
