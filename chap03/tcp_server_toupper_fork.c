/* tcp_server_toupper_fork.c */

#if defined(_WIN32)
#error This program does not support Windows.
#endif

#include "chap03.h"
#include <ctype.h>

/* The socket API is blocking by nature. When you call functions like accept(),
 * recv() or send() the whole program will block waiting for them to return.
 * This behavior is a non-starter when you need to service multiple in-coming
 * connections. One solution to cope with this blocking behaviour is implementing
 * multi threading/process techniques with the fork() function. Obviously this
 * solution only applies to Unix mike systems. */
int main(void) {

  // Configure local address
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(0, "3750", &hints, &bind_address);

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
  freeaddrinfo(bind_address); // Free address info memory

  // Listen for connections
  printf("Listening for connections...\n");
  if (listen(socket_listen, 10) < 0) {
    fprintf(stderr, "listen() failed with error %d\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  // Await connections
  printf("Waiting for connections...\n");

  while (1) {

    // Accept a connection
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    SOCKET socket_client;
    socket_client = accept(socket_listen, (struct sockaddr *)&client_addr,
                           &client_addr_len);
    if (!ISVALIDSOCKET(socket_client)) {
      fprintf(stderr, "accept() failed with error %d\n", GETSOCKETERRNO());
      return EXIT_FAILURE;
    }

    // Print client address information
    char addr_buf[100];
    getnameinfo((struct sockaddr *)&client_addr, client_addr_len, addr_buf,
                sizeof(addr_buf), 0, 0, NI_NUMERICHOST);
    printf("New connection from %s\n", addr_buf);

    int pID = fork();

    if (pID == 0) { // Child process
      // Close listening socket
      CLOSESOCKET(socket_listen);

      while (1) {
        char readbuf[1024];
        // Receive data
        int bytes_received = recv(socket_client, readbuf, 1024, 0);
        if (bytes_received < 1) {
          CLOSESOCKET(socket_client);
          exit(EXIT_SUCCESS);
        }
        // Process data
        for (int j = 0; j < bytes_received; j++) {
          readbuf[j] = toupper(readbuf[j]);
        }
        // Send data
        send(socket_client, readbuf, bytes_received, 0);
      } // Inner infinite while(1) loop

    } // if pID == 0

    CLOSESOCKET(socket_client);

  } // Outer infinte while(1) loop

  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen);

  printf("Finished.\n");

  return EXIT_SUCCESS;
}
