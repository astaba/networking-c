/* web_server.c */

#include "chap07.h"

#define MAX_REQUEST_SIZE 2047

typedef struct client_info {
  socklen_t address_length;
  struct sockaddr_storage address;
  SOCKET socket;
  char request[MAX_REQUEST_SIZE + 1];
  int received;
  struct client_info *next;
} client_info;

static client_info *clients = NULL;

client_info *get_client(SOCKET socket);
void drop_client(client_info *client);
const char *get_client_address(client_info *client);
void send_400(client_info *client);
void send_404(client_info *client);
void serve_resource(client_info *client, const char *path);

int main(void) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return EXIT_FAILURE;
  }
#endif

  // Create listen socket
  SOCKET server;
  {
    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    char *port = "3158";
    getaddrinfo(0, port, &hints, &bind_address);
    // If you want to accept connections from the local system only
    /* getaddrinfo("127.0.0.1", "3157", &hints, &bind_address); */

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

    server = socket_listen;
  }

  while (1) {

    // Wait for clients
    fd_set readfds;
    {
      FD_ZERO(&readfds);
      FD_SET(server, &readfds);
      SOCKET max_socket = server;

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
    }

    if (FD_ISSET(server, &readfds)) {
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

client_info *get_client(SOCKET socket) {
  client_info *client = clients;
  while (client) {
    if (client->socket == socket)
      return client;
    client = client->next;
  }

  client = (client_info *)calloc(1, sizeof(client_info));
  if (!client) {
    fprintf(stderr, "Out of memory.\n");
    exit(EXIT_FAILURE);
  }
  client->address_length = sizeof(client->address);
  client->next = clients;
  clients = client;
  return client;
}

void drop_client(client_info *client) {
  CLOSESOCKET(client->socket);
  client_info **ptr = &clients;
  while (*ptr) {
    if (*ptr == client) {
      *ptr = client->next;
      free(client);
      return;
    }
    ptr = &(*ptr)->next;
  }

  fprintf(stderr, "drop_client not found.\n");
  exit(EXIT_FAILURE);
}

const char *get_client_address(client_info *client) {
  static char address_buffer[100];
  getnameinfo((struct sockaddr *)&client->address, client->address_length,
              address_buffer, sizeof(address_buffer), NULL, 0, NI_NUMERICHOST);
  return address_buffer;
}

void send_400(client_info *client) {
  const char *buf_400 = "HTTP/1.1 400 Bad Request\r\n"
                        "Connection: close\r\n"
                        "Content-Length: 11\r\n\r\n"
                        "Bad Request";
  send(client->socket, buf_400, strlen(buf_400), 0);
  drop_client(client);
}

void send_404(client_info *client) {
  const char *buf_404 = "HTTP/1.1 404 Not Found\r\n"
                        "Connection: close\r\n"
                        "Content-Length: 9\r\n\r\n"
                        "Not Found";
  send(client->socket, buf_404, strlen(buf_404), 0);
  drop_client(client);
}

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

  FILE *fp = fopen(full_path, "rb");
  if (!fp) {
    send_404(client);
    return;
  }

  fseek(fp, 0L, SEEK_END);
  size_t cl = ftell(fp);
  rewind(fp);

  const char *ct;
  {
    const char *last_dot = strrchr(full_path, '.');
    if (last_dot) {
      // clang-format off
      if (strcmp(last_dot, ".css") == 0)       ct = "text/css";
      else if (strcmp(last_dot, ".csv") == 0)  ct = "text/csv";
      else if (strcmp(last_dot, ".htm") == 0)  ct = "text/html";
      else if (strcmp(last_dot, ".html") == 0) ct = "text/html";
      else if (strcmp(last_dot, ".txt") == 0)  ct = "text/plain";
      else if (strcmp(last_dot, ".gif") == 0)  ct = "image/gif";
      else if (strcmp(last_dot, ".ico") == 0)  ct = "image/x-icon";
      else if (strcmp(last_dot, ".jpeg") == 0) ct = "image/jpeg";
      else if (strcmp(last_dot, ".jpg") == 0)  ct = "image/jpeg";
      else if (strcmp(last_dot, ".png") == 0)  ct = "image/png";
      else if (strcmp(last_dot, ".svg") == 0)  ct = "image/svg+xml";
      else if (strcmp(last_dot, ".js") == 0)   ct = "application/javascript";
      else if (strcmp(last_dot, ".json") == 0) ct = "application/json";
      else if (strcmp(last_dot, ".pdf") == 0)  ct = "application/pdf";
      else if (strcmp(last_dot, ".xml") == 0)  ct = "application/xml";
      // clang-format on
    } else {
      ct = "application/octet-stream";
    }
  }

#define BSIZE 1024
  char buffer[BSIZE];

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

  size_t r;
  while ((r = fread(buffer, 1, BSIZE, fp)) > 0)
    send(client->socket, buffer, r, 0);

  // TEST:
  printf("serve_resource completed %s %s\n", get_client_address(client), path);

  fclose(fp);
  drop_client(client);
}
