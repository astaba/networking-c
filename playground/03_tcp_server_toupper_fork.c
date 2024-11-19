/* 03_tcp_server_toupper_fork.c */

#if defined(_WIN32)
#error "This program does not support Windows."
#endif

#include <errno.h>      // Error handling
#include <netdb.h>      // DNS-related functions
#include <unistd.h>     // UNIX-specific functions (e.g., close)
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The socket API is blocking by nature. When you call functions like accept(),
 * recv() or send() the whole program will block waiting for them to return.
 * This behavior is a non-starter when you need to service multiple in-coming
 * connections. One solution to cope with this blocking behaviour is
 * implementing multi threading/process techniques with the fork() function.
 * Obviously this solution only applies to Unix mike systems. */
int main(void) {

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
  int socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (socket_listen < 0) {
    fprintf(stderr, "socket() failed. (%d)\n", errno);
    exit(EXIT_FAILURE);
  }

  // Make the socket dual stack
  int option = 0;
  if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&option,
                 sizeof(option))) {
    fprintf(stderr, "setsocketoopt() failed. (%d)\n", errno);
    exit(EXIT_FAILURE);
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed. (%d)\n", errno);
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Listening...\n");
  if (listen(socket_listen, 10) < 0) {
    fprintf(stderr, "listen() failed. (%d)\n", errno);
    exit(EXIT_FAILURE);
  }
  printf("Server listnening on port %s.\n", port);

  printf("Waiting for connections...\n");

  while (1) {
    struct sockaddr_storage client_address;
    socklen_t client_addr_len = sizeof(client_address);
    int socket_client = accept(
        socket_listen, (struct sockaddr *)&client_address, &client_addr_len);
    if (socket_client < 0) {
      fprintf(stderr, "accept() failed. (%d)\n", errno);
      exit(EXIT_FAILURE);
    }

    printf("New connection from: ");
    char address_buffer[100];
    char service_buffer[100];
    getnameinfo((struct sockaddr *)&client_address, client_addr_len,
                address_buffer, sizeof(address_buffer), service_buffer,
                sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
    printf("%s:%s\n", address_buffer, service_buffer);

    int pid = fork();

    if (pid == 0) {
      close(socket_listen);

      while (1) {
        char read[1024];
        int bytes_received = recv(socket_client, read, 1024, 0);

        if (bytes_received < 1) {
          close(socket_client);
          exit(EXIT_SUCCESS);
        }

        int j;
        for (j = 0; j < bytes_received; j++) {
          read[j] = toupper(read[j]);
        }

        send(socket_client, read, bytes_received, 0);
      }
    }

    close(socket_client);
  } // while(1)

  // INFO: Du to the preceding while loop the next part of the program will
  // never run. Nevertheless, is it a good practice in the event we later add
  // functionality to abort the while loop.

  // Cleanup routines
  printf("Close listening socket...\n");
  close(socket_listen);

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
