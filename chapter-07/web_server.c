/* web_server.c */

#include "chap07.h"

#define MAX_REQUEST_SIZE 2047

// Struct to store information about each connected client in linked list
typedef struct client_info {
  socklen_t address_length;
  struct sockaddr_storage address;
  SOCKET socket;
  char request[MAX_REQUEST_SIZE + 1];
  int received;
  struct client_info *next;
} client_info;

// Make the head of the linked list globally available
static client_info *clients = NULL;

// Helper functions prototypes
SOCKET create_socket(const char *host, const char *port);
fd_set wait_on_clients(SOCKET socket_listen);
const char *get_client_address(client_info *client);
client_info *get_client(SOCKET socket);
void send_400(client_info *client);
void send_404(client_info *client);
void drop_client(client_info *client);
const char *get_content_type(const char *path);
void serve_resource(client_info *client, const char *path);

/**
 * @brief Main entry point for the web server.
 *
 * The web server uses the BSD sockets API to create a socket and listen for
 * incoming connections. It processes the HTTP request by reading the request
 * line and headers, and serving the requested resource. If the request line is
 * malformed or the resource does not exist, it sends an appropriate HTTP
 * response code.
 *
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error.
 */
int main(void) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return EXIT_FAILURE;
  }
#endif

  SOCKET server = create_socket(0, "3157");
  // If you want to accept connections from the local system only
  /* SOCKET server = create_socket("127.0.0.1", "3157"); */

  while (1) {
    fd_set readfds;
    readfds = wait_on_clients(server);

    if (FD_ISSET(server, &readfds)) {
      // HACK: -1 being an invalid socket specifier it forces get_client to
      // return a pointer to a freshly allocated client_info.
      client_info *client = get_client(-1);

      client->socket = accept(server, (struct sockaddr *)&client->address,
                              &client->address_length);
      if (!ISVALIDSOCKET(client->socket)) {
        fprintf(stderr, "accept() failed. (%d)\n", GETSOCKETERRNO());
        return EXIT_FAILURE;
      }

      printf("New connection from %s.\n", get_client_address(client));
    } // if (FD_ISSET(server, &readfds))

    client_info *client = clients;
    while (client) {
      client_info *next = client->next;

      if (FD_ISSET(client->socket, &readfds)) {
        if (MAX_REQUEST_SIZE == client->received) {
          send_400(client);
          continue;
        }

        int bytes_received =
            recv(client->socket, client->request + client->received,
                 MAX_REQUEST_SIZE - client->received, 0);

        // TEST: Monitor packets split across multiple recv() calls
        /* printf("\nReceived Data (%d bytes) ->>\n%.*s\n<<-\n", bytes_received,
               bytes_received, client->request); */

        if (bytes_received < 1) {
          printf("Unexpected disconnect from %s.\n",
                 get_client_address(client));
          drop_client(client);
        } else {
          client->received += bytes_received;
          client->request[client->received] = 0;

          char *q = strstr(client->request, "\r\n\r\n");
          if (q) {
            if (strncmp("GET /", client->request, 5)) {
              send_400(client);
            } else {
              char *path = client->request + 4;
              char *end_path = strstr(path, " ");
              if (!end_path) {
                send_400(client);
              } else {
                *end_path = 0;
                serve_resource(client, path);
              }
            } // if request line starts with: "GET /"
          } // if (q)
        } // if (bytes_received >= 1)
      } // if (FD_ISSET(client->socket, &readfds))

      client = next;
    } // while(client)
  } // while(1)

  // Cleanup routines
  printf("\nClosing socket...\n");
  CLOSESOCKET(server);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}

/**
 * @brief Create a socket and configure it to listen on a given host and port.
 *
 * The function configures a local address and port for the server, binds a
 * socket to it, and puts it into listening mode.
 *
 * @param host The host where the server is listening
 * @param port The port where the server is listening
 * @return The socket that is listening
 */
SOCKET create_socket(const char *host, const char *port) {
  // Configure local address the server is listening on
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(host, port, &hints, &bind_address);

  // Create socket
  printf("Creating socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  // Bind socket to local address
  printf("Bind socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  // Put socket into listening mode
  printf("Listening at http://127.0.0.1:%s/\n", port);
  if (listen(socket_listen, 10) < 0) {
    fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  return socket_listen;
}

/**
 * @brief Wait for incoming data from any of the existing clients.
 *
 * This function will block until an existing client sends data, or a new client
 * attempts to connect. It should be called in a loop to handle incoming data.
 *
 * @param socket_listen The socket to listen on.
 * @return A file descriptor set indicating which sockets have data waiting.
 */
fd_set wait_on_clients(SOCKET socket_listen) {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(socket_listen, &readfds);
  SOCKET max_socket = socket_listen;

  client_info *client = clients;

  // Add all client sockets to the file descriptor set
  while (client) {
    FD_SET(client->socket, &readfds);
    if (client->socket > max_socket)
      max_socket = client->socket;
    client = client->next;
  }

  if (select(max_socket + 1, &readfds, 0, 0, 0) < 0) {
    fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  return readfds;
}

/**
 * @brief Converts a client_info struct into a string representation of the IP
 *        address.
 *
 * This function is a helper that converts the IP address stored in a
 * client_info struct into a string. The string is stored in a static buffer and
 * is overwritten each time the function is called. Do not use this function
 * concurrently since it is not re-entrant safe.
 *
 * @param client The client_info pointer to convert.
 * @return A string representation of the client's IP address.
 */
const char *get_client_address(client_info *client) {
  static char address_buffer[100];
  getnameinfo((struct sockaddr *)&client->address, client->address_length,
              address_buffer, sizeof(address_buffer), NULL, 0, NI_NUMERICHOST);
  return address_buffer;
}

/**
 * @brief Retrieve connected client_info structure for a given socket.
 *
 * This function searches the linked list of connected clients to find the
 * client_info associated with the specified socket. If no such client_info
 * exists, a new one is allocated and added to the list.
 *
 * @param socket The socket associated with the client to retrieve.
 * @return Either a pointer the connected client_info or to an unconnected
 * client_info newly allocated.
 */
client_info *get_client(SOCKET socket) {
  client_info *client = clients;
  // Search for the connected client_info in the linked list
  while (client) {
    if (client->socket == socket)
      return client;
    client = client->next;
  }

  // NOTE: This section of the function is responsible for giving birth to the
  // whole linked list from the first node, pointed to by clients, to all
  // subsequent prepended nodes.

  // Allocate a new client_info if not found
  client = (client_info *)calloc(1, sizeof(client_info));
  if (!client) {
    fprintf(stderr, "Out of memory.\n");
    exit(EXIT_FAILURE);
  }
  // Initialize the new client_info and add it to the list.
  client->address_length = sizeof(client->address);
  client->next = clients;
  clients = client;
  return client;
}

/**
 * @brief Send a 400 error response to the client and close the connection.
 *
 * This function sends a 400 error response to the client and then closes the
 * connection.
 *
 * @param client The client to send the error response to.
 */
void send_400(client_info *client) {
  const char *buf_400 = "HTTP/1.1 400 Bad Request\r\n"
                        "Connection: close\r\n"
                        "Content-Length: 11\r\n\r\n"
                        "Bad Request";
  send(client->socket, buf_400, strlen(buf_400), 0);
  drop_client(client);
}

/**
 * @brief Send a 404 error response to the client and close the connection.
 *
 * This function sends a 404 error response to the client and then closes the
 * connection.
 *
 * @param client The client to send the error response to.
 */
void send_404(client_info *client) {
  const char *buf_404 = "HTTP/1.1 404 Not Found\r\n"
                        "Connection: close\r\n"
                        "Content-Length: 9\r\n\r\n"
                        "Not Found";
  send(client->socket, buf_404, strlen(buf_404), 0);
  drop_client(client);
}

/**
 * @brief Free a client_info struct from the linked list.
 *
 * This function searches for the specified client_info pointer in the linked
 * list and removes it, effectively disconnecting the client. If the client_info
 * is not found, the program exitsiwith an error.
 *
 * @param client The pointer to the client_info to remove.
 */
void drop_client(client_info *client) {
  // Close the socket
  CLOSESOCKET(client->socket);

  // Search the linked list for the client_info
  client_info **ptr = &clients;

  while (*ptr) {
    if (*ptr == client) {
      // Client_info was found, remove it from the list
      *ptr = client->next;
      free(client);
      return;
    }
    ptr = &(*ptr)->next;
  }

  // Client_info was not found, exit with error
  fprintf(stderr, "drop_client not found.\n");
  exit(EXIT_FAILURE);
}

/**
 * @brief Return the MIME type of a file given its path.
 *
 * If the file type is not recognized, the function returns
 * "application/octet-stream".
 *
 * @param path The path of the file
 * @return The MIME type of the file
 */
const char *get_content_type(const char *path) {
  const char *last_dot = strrchr(path, '.');
  if (last_dot) {
    // clang-format off
    if (strcmp(last_dot, ".css")  == 0) return "text/css";
    if (strcmp(last_dot, ".csv")  == 0) return "text/csv";
    if (strcmp(last_dot, ".htm")  == 0) return "text/html";
    if (strcmp(last_dot, ".html") == 0) return "text/html";
    if (strcmp(last_dot, ".txt")  == 0) return "text/plain";
    if (strcmp(last_dot, ".gif")  == 0) return "image/gif";
    if (strcmp(last_dot, ".ico")  == 0) return "image/x-icon";
    if (strcmp(last_dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(last_dot, ".jpg")  == 0) return "image/jpeg";
    if (strcmp(last_dot, ".png")  == 0) return "image/png";
    if (strcmp(last_dot, ".svg")  == 0) return "image/svg+xml";
    if (strcmp(last_dot, ".js")   == 0) return "application/javascript";
    if (strcmp(last_dot, ".json") == 0) return "application/json";
    if (strcmp(last_dot, ".pdf")  == 0) return "application/pdf";
    if (strcmp(last_dot, ".xml")  == 0) return "application/xml";
    // clang-format on
  }
  return "application/octet-stream";
}

/**
 * @brief Send a resource to the client.
 *
 * This function takes a connected client and requested resource path then send
 * the connected client the requested resource. The server expects all hosted
 * files to be in a subdirectory called public. Ideally, the server should not
 * allow access to any files outside of this public directory. However, as we
 * shall see, enforcing this restriction may be more difficult than it first
 * appears.
 *
 * @param client The client to send the resource to.
 * @param path The path of the resource to send.
 */
void serve_resource(client_info *client, const char *path) {
  // DEBUG: Production servers would print at least date, time, request method,
  // client's user-agent string and response code...
  printf("serve_resource initiated %s %s\n", get_client_address(client), path);

  // Redirect root request and prevent long or obviously malicious requests:
  if (strcmp(path, "/") == 0)
    path = "/index.html";
  if (strlen(path) > 100) {
    send_400(client);
    return;
  }
  if (strstr(path, "..")) {
    send_404(client);
    return;
  }

  // Convert path to refer to files in the public directory
  char full_path[128];
  snprintf(full_path, 128, "public%s", path);
#if defined(_WIN32)
  char *p = full_path;
  while (*p) {
    if (*p == '/')
      *p = '\\';
    p++;
  }
#endif

  // Try to open binary stream for the resource, and in case of failure the
  // server assumes it doesn't exist.
  FILE *fp = fopen(full_path, "rb");
  if (!fp) {
    send_404(client);
    return;
  }

  // Retrieve metadata to populate the Content-Length header
  fseek(fp, 0L, SEEK_END);
  size_t cl = ftell(fp);
  rewind(fp);
  // Retrieve metadata to populate the Content-Type header
  const char *ct = get_content_type(full_path);

#define BSIZE 1024
  char buffer[BSIZE];

  // Send the response headers
  snprintf(buffer, BSIZE, "HTTP/1.1 200 OK\r\n");
  send(client->socket, buffer, strlen(buffer), 0);
  snprintf(buffer, BSIZE, "Connection: close\r\n");
  send(client->socket, buffer, strlen(buffer), 0);
  snprintf(buffer, BSIZE, "Content-Length: %zu\r\n", cl);
  send(client->socket, buffer, strlen(buffer), 0);
  snprintf(buffer, BSIZE, "Content-Type: %s\r\n", ct);
  send(client->socket, buffer, strlen(buffer), 0);
  snprintf(buffer, BSIZE, "\r\n");
  send(client->socket, buffer, strlen(buffer), 0);

  // Send the body
  size_t r;
  while ((r = fread(buffer, 1, BSIZE, fp)) > 0)
    send(client->socket, buffer, r, 0);

  // TEST:
  printf("serve_resource completed %s %s\n", get_client_address(client), path);

  // Close file handle and disconnect client
  fclose(fp);
  drop_client(client);
}
