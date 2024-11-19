/* time_server.c */

#include "chap02.h"

int main(void) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif

  printf("Configuring local address...\n");
  struct addrinfo hints;            // Hints for address configuration
  memset(&hints, 0, sizeof(hints)); // Zero out the structure
  hints.ai_family = AF_INET;        // IPv4 family
  hints.ai_socktype = SOCK_STREAM;  // Stream socket (TCP)
  hints.ai_flags = AI_PASSIVE;      // Bind to any available address
  struct addrinfo *bind_address;    // Pointer to store address information
  // Get local address info for port 80 (HTTP)
  getaddrinfo(0, "8080", &hints, &bind_address);

  printf("Creating socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }
  freeaddrinfo(bind_address); // Free the address info memory

  printf("Listening...\n");
  // Start listening for incoming connections, backlog of 10
  if (listen(socket_listen, 10) < 0) {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  printf("Waiting for connection...\n");
  // Structure to hold the client's address
  struct sockaddr_storage client_address;
  socklen_t client_len = sizeof(client_address);
  // Blocking call to accept an incoming connection
  SOCKET socket_client =
      accept(socket_listen, (struct sockaddr *)&client_address, &client_len);
  if (!ISVALIDSOCKET(socket_client)) {
    fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  printf("Client is connected... ");
  char address_buffer[100]; // Buffer to hold the client's address as a string
  // Get the client's IP address
  getnameinfo((struct sockaddr *)&client_address, client_len, address_buffer,
              sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
  printf("%s\n", address_buffer); // Print the client's IP address

  printf("Reading request...\n");
  char request[1024]; // Buffer to store the client's request
  // Receive data from the client
  int bytes_received = recv(socket_client, request, 1024, 0);
  // Print how many bytes were received
  printf("Received %d bytes.\n", bytes_received);
  // Print the received request
  printf("%.*s", bytes_received, request);

  printf("Sending response...\n");
  // Prepare an HTTP response headers and introduce response body
  const char *response = "HTTP/1.1 200 OK\r\n"
                         "Connection: close\r\n"
                         "Content-Type: text/plain\r\n\r\n"
                         "Local time is: ";
  // Send the HTTP headers and initial response
  int bytes_sent = send(socket_client, response, strlen(response), 0);
  // Print how many bytes were sent
  printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));

  // Prepare the response body
  time_t timer;
  time(&timer);
  char *time_msg = ctime(&timer);
  // Send the time string to the client
  bytes_sent = send(socket_client, time_msg, strlen(time_msg), 0);
  // Print how many bytes were sent
  printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

  // Close client's socket to prevent browser from waiting until it times out
  printf("Closing connection...\n");
  CLOSESOCKET(socket_client);

  // At this point a production server would accept additional connections by
  // calling accept() once again.
  // printf("Waiting for next connection...\n");

  printf("Closing listening socket...\n");
  CLOSESOCKET(socket_listen); // Close the listening socket

#if defined(_WIN32)
  WSACleanup(); // Clean up Winsock on Windows
#endif

  printf("Finished.\n");

  return EXIT_SUCCESS; // Exit successfully
}
