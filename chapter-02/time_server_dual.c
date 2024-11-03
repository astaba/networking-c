/* time_server.c */

// Platform-specific headers for network programming

// If the program is being compiled on Windows
#if defined(_WIN32)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Target Windows version is Vista or higher
#endif
#include <winsock2.h> // Windows Sockets API
#include <ws2tcpip.h> // Extensions for modern IP and DNS support
#pragma comment(lib,                                                           \
                "ws2_32.lib") // Link the Winsock library during compilation

#else // If the program is being compiled on a UNIX-like system
#include <arpa/inet.h>  // Functions for working with IP addresses
#include <errno.h>      // Error handling
#include <netdb.h>      // DNS-related functions
#include <netinet/in.h> // Structures for handling internet addresses
#include <sys/socket.h> // Socket-related functions
#include <sys/types.h>  // Defines data types used in system calls
#include <unistd.h>     // UNIX standard functions (e.g., close)

#endif

// Platform-specific macros to handle differences between Windows and UNIX-like
// systems

#if defined(_WIN32) // Windows-specific definitions
#define ISVALIDSOCKET(s)                                                       \
  ((s) != INVALID_SOCKET)             // Check if a socket is valid on Windows
#define CLOSESOCKET(s) closesocket(s) // Close a socket on Windows
#define GETSOCKETERRNO()                                                       \
  (WSAGetLastError()) // Get the last error code on Windows

#else                               // UNIX-specific definitions
#define ISVALIDSOCKET(s) ((s) >= 0) // Check if a socket is valid on UNIX
#define CLOSESOCKET(s) close(s)     // Close a socket on UNIX
#define SOCKET int // Sockets are just file descriptors (int) on UNIX
#define GETSOCKETERRNO() (errno) // Get the last error code on UNIX

#endif

#include <stdio.h>  // Standard input/output
#include <stdlib.h> // Standard library functions (e.g., exit)
#include <string.h> // String manipulation
#include <time.h>   // Time-related functions

/**
 * The main function configures a local address and port for the server, binds a
 * socket to it, listens for incoming connections, accepts a connection, reads a
 * request from the client, sends back a response with the current local time,
 * and closes the connection.
 *
 * @return EXIT_SUCCESS on successful completion, EXIT_FAILURE otherwise.
 */
int main(void) {

#if defined(_WIN32) // Windows-specific initialization
  WSADATA d;        // Structure to hold Windows Sockets data
  if (WSAStartup(MAKEWORD(2, 2), &d)) { // Initialize Winsock version 2.2
    fprintf(stderr, "Failed to initialize.\n");
    return 1;
  }
#endif

  printf("Configuring local address...\n");
  struct addrinfo hints;            // Hints for address configuration
  memset(&hints, 0, sizeof(hints)); // Zero out the structure
  hints.ai_family = AF_INET6;       // IPv4 family
  hints.ai_socktype = SOCK_STREAM;  // Stream socket (TCP)
  hints.ai_flags = AI_PASSIVE;      // Bind to any available address
  struct addrinfo *bind_address;    // Pointer to store address information
  // Get local address info for port 80
  getaddrinfo(0, "80", &hints, &bind_address);

  printf("Creating socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  int option = 0;
  if (setsockopt(socket_listen, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&option,
                 sizeof(option))) {
    fprintf(stderr, "setsockopt() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
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
  struct sockaddr_storage client_address; // Structure to hold the client's address
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

  printf("Closing connection...\n");
  // Close the client's socket for the browser to keep waiting until times out
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
