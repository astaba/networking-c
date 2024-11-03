/* tcp_server_toupper.c */

#include "../mylib/omniplat.h"
#include <ctype.h>
#include <stdlib.h>

int main(void) {

  // Initialize Winsock
#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return EXIT_FAILURE;
  }
#endif

  // Configure local address
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  // Call getaddrinfo and make sure to use the retunr value
  if (getaddrinfo(0, "3750", &hints, &bind_address)) {
    fprintf(stderr, "getaddrinfo() failed with error %d\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  // Create socket
  printf("Creating socket...\n");
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
    fprintf(stderr, "bind() failed with error %d\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }
  freeaddrinfo(bind_address); // Free the address info memory

  // Listen for connections
  printf("Listening for connections...\n");
  if (listen(socket_listen, 10) < 0) {
    fprintf(stderr, "listen() failed with error %d\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  // Set up master socket set
  fd_set master;
  FD_ZERO(&master);
  FD_SET(socket_listen, &master);
  SOCKET max_socket = socket_listen;

  // Wait for connections
  printf("Waiting for connections...\n");
  while (1) {
    fd_set readfds = master;
    if (select(max_socket + 1, &readfds, NULL, NULL, NULL) < 0) {
      fprintf(stderr, "select() failed with error %d\n", GETSOCKETERRNO());
      return EXIT_FAILURE;
    }

    SOCKET i;
    for (i = 1; i <= max_socket; i++) {
      if (FD_ISSET(i, &readfds)) {
        if (i == socket_listen) { // Then accept a connection
          struct sockaddr_storage client_address;
          socklen_t client_len = sizeof(client_address);
          SOCKET socket_client = accept(
              socket_listen, (struct sockaddr *)&client_address, &client_len);
          if (!ISVALIDSOCKET(socket_client)) {
            fprintf(stderr, "accept() failed with error %d\n",
                    GETSOCKETERRNO());
            return EXIT_FAILURE;
          }
          // Add the socket to the master set and maintain max fd
          FD_SET(socket_client, &master);
          if (socket_client > max_socket)
            max_socket = socket_client;

          // Print client address information
          char address_buffer[100];
          getnameinfo((struct sockaddr *)&client_address, client_len,
                      address_buffer, 100, NULL, 0, NI_NUMERICHOST);
          printf("New connection from %s on socket %d\n", address_buffer,
                 (int)socket_client);
        } else { // It is instead a request for an established connection
          char recvbuf[1024];
          // Receive data from client
          int bytes_received = recv(i, recvbuf, 1024, 0);
          if (bytes_received < 1) { // then client closed connection
            FD_CLR(i, &master);
            CLOSESOCKET(i);
            continue;
          }
          // Process received data
          int j;
          for (j = 0; j < bytes_received; j++)
            recvbuf[j] = toupper(recvbuf[j]);
          // Send data back to client
          send(i, recvbuf, bytes_received, 0);
        }
      } // end of if FD_ISSET
    } // end of for i to max_socket
  } // end of while(1)

  // Close the listening socket
  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
