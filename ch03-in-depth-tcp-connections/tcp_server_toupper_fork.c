// ch03-in-depth-tcp-connections/tcp_server_toupper_fork.c

#if defined(_WIN32)
#error This program does not support Windows.
#endif

#include "chap03.h"

/* The socket API is blocking by nature. When you call functions like accept(),
 * recv() or send() the whole program will block waiting for them to return.
 * This behavior is a non-starter when you need to service multiple in-coming
 * connections. One solution to cope with this blocking behaviour is
 * implementing multi threading/process techniques with the fork() function.
 * Obviously this solution only applies to Unix mike systems. */
int main(void) {

  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(0, "8080", &hints, &bind_address);

  printf("Creating socket...\n");
  int socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                             bind_address->ai_protocol);
  if (socket_listen < 0) {
    perror("socket() failed");
    exit(EXIT_FAILURE);
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    perror("bind() failed");
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Listening for connections...\n");
  if (listen(socket_listen, 10) < 0) {
    perror("listen() failed");
    exit(EXIT_FAILURE);
  }

  printf("Waiting for connections...\n");
  while (1) {
    struct sockaddr_storage client_address;
    socklen_t client_len = sizeof(client_address);
    int socket_client =
        accept(socket_listen, (struct sockaddr *)&client_address, &client_len);
    if (socket_client < 0) {
      perror("accept() failed");
      exit(EXIT_FAILURE);
    }

    char address_buf[100];
    getnameinfo((struct sockaddr *)&client_address, client_len, address_buf,
                sizeof(address_buf), NULL, 0, NI_NUMERICHOST);
    printf("New connection from %s on socket %d\n", address_buf, socket_listen);

    int pid;
    pid = fork();

    if (pid == 0) { // Then we are in the child process
      close(socket_listen);

      while (1) {
        // Receive data
        char read_buffer[1024];
        int bytes_received = recv(socket_client, read_buffer, 1024, 0);
        if (bytes_received < 1) {
          close(socket_client);
          exit(EXIT_SUCCESS);
        }
        // Run ENGINE
        for (size_t j = 0; j < bytes_received; j++) {
          read_buffer[j] = toupper(read_buffer[j]);
        }
        // send() response data
        send(socket_client, read_buffer, bytes_received, 0);
      }
    }
    // Otherwise we are still in the parent process
    close(socket_client);
  }

  printf("Closing listening socket...\n");
  close(socket_listen);

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
