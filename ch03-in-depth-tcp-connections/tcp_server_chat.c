// ch03-in-depth-tcp-connections/tcp_server_chat.c

#include "chap03.h"

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
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(0, "8080", &hints, &bind_address);

  printf("Creating socket...\n");
  SOCKET socket_listen =
      socket(bind_address->ai_family, bind_address->ai_socktype,
             bind_address->ai_protocol);
  if (BAD_SOCKET(socket_listen)) {
    REPORT_SOCKET_ERROR("socket() failed");
    exit(EXIT_FAILURE);
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    REPORT_SOCKET_ERROR("bind() failed");
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Listening for connections...\n");
  if (listen(socket_listen, 10) < 0) {
    REPORT_SOCKET_ERROR("listen() failed");
    exit(EXIT_FAILURE);
  }

  // Set the read socket set
  fd_set masterfds;
  FD_ZERO(&masterfds);
  FD_SET(socket_listen, &masterfds);
  SOCKET max_socket = socket_listen;

  // Wait for connections
  printf("Waiting for connections...\n");
  while (1) {
    fd_set readfds = masterfds;
    if (select(max_socket + 1, &readfds, NULL, NULL, NULL) < 0) {
      REPORT_SOCKET_ERROR("select() failed");
      exit(EXIT_FAILURE);
    }

    SOCKET i;
    for (i = 1; i <= max_socket; i++) {
      if (FD_ISSET(i, &readfds)) {
        if (i == socket_listen) { // Then accept() connection
          struct sockaddr_storage client_address;
          socklen_t client_len = sizeof(client_address);
          SOCKET socket_client = accept(
              socket_listen, (struct sockaddr *)&client_address, &client_len);
          if (BAD_SOCKET(socket_client)) {
            REPORT_SOCKET_ERROR("accept() failed");
            exit(EXIT_FAILURE);
          }

          FD_SET(socket_client, &masterfds);
          if (socket_client > max_socket) {
            max_socket = socket_client;
          }

          char address_buf[100];
          getnameinfo((struct sockaddr *)&client_address, client_len,
                      address_buf, 100, NULL, 0, NI_NUMERICHOST);
          printf("New connection from %s on socket %d\n", address_buf,
                 socket_client);

        } else { // otherwise recv() request from established connection
          // recv() request data
          char recv_buf[1024];
          int bytes_received = recv(i, recv_buf, 1024, 0);
          if (bytes_received < 1) {
            FD_CLR(i, &masterfds);
            CLOSESOCKET(i);
            continue;
          }
          // Run the ENGINE
          for (SOCKET j = 1; j <= max_socket; j++) {
            if (FD_ISSET(j, &masterfds)) {
              if (j == socket_listen || j == i) {
                continue;
              } else {
                // send() response to chat member
                send(j, recv_buf, bytes_received, 0);
              }
            }
          }
        } // ifelse (i == socket_listen)
      } // if (FD_ISSET(i, &readfds))
    } // for (i = 1; i <= max_socket; i++)
  } // while(1)

  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);

#ifdef _WIN32
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
