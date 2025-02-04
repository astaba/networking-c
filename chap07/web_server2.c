/* web_server2.c */

#include "chap07.h"

#define MAX_REQUEST_SIZE (2 * 1024 - 1)

typedef struct client_info {
  socklen_t address_length;
  struct sockaddr_storage address;
  char info_buf[128];
  SOCKET socket;
  char request[MAX_REQUEST_SIZE + 1];
  int received;
  struct client_info *next;
} client_info;

SOCKET create_socket(const char *host, const char *port);
fd_set wait_on_clients(client_info *clients_head, SOCKET server);
client_info *get_client(client_info **clients_head, SOCKET socket);
void send_400(client_info **clients_head, client_info *client);
void send_404(client_info **clients_head, client_info *client);
void drop_client(client_info **clients_head, client_info *client);
const char *get_content_type(const char *path);
void serve_resource(client_info **clients_head, client_info *client,
                    const char *path);

int main(void) {

#if defined(_WIN32)
  WSADATA d;
  if (WASStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    return EXIT_FAILURE;
  }
#endif

  SOCKET server = create_socket(0, "3158");

  client_info *clients_head = 0;
  // Main loop
  while (1) {
    // Wait for incoming connections
    fd_set readfds;
    readfds = wait_on_clients(clients_head, server);

    if (FD_ISSET(server, &readfds)) {
      // Get a new client_info
      client_info *client = get_client(&clients_head, -1);

      client->socket = accept(server, (struct sockaddr *)&client->address,
                              &client->address_length);
      if (!ISVALIDSOCKET(client->socket)) {
        fprintf(stderr, "accept() failed with error %d\n", GETSOCKETERRNO());
        exit(EXIT_FAILURE);
      }
      // Cache formatted address of client
      getnameinfo((struct sockaddr *)&client->address, client->address_length,
                  client->info_buf, sizeof(client->info_buf), 0, 0,
                  NI_NUMERICHOST);

      printf("New Connection from %s\n", client->info_buf);
    }

    client_info *client = clients_head;
    while (client) {
      client_info *next_client = client->next;

      if (FD_ISSET(client->socket, &readfds)) {

        if (client->received == MAX_REQUEST_SIZE) {
          send_400(&clients_head, client);
          client = next_client;
          continue;
        }

        int received = recv(client->socket, client->request + client->received,
                            MAX_REQUEST_SIZE - client->received, 0);

        if (received < 1) {
          printf("Unexpected disconnect from %s.\n", client->info_buf);
          drop_client(&clients_head, client);

        } else {
          client->received += received;
          client->request[client->received] = '\0';

          char *headers_terminator = "\r\n\r\n";
          char *headers_end = strstr(client->request, headers_terminator);
          if (headers_end) {
            *headers_end = '\0';
            char *request_type = "GET /";

            if (strncmp(request_type, client->request, strlen(request_type))) {
              send_400(&clients_head, client);

            } else {
              char *path = client->request + strlen(headers_terminator);
              char *path_end = strstr(path, " ");
              if (!path_end) {
                send_400(&clients_head, client);

              } else {
                *path_end = 0;
                serve_resource(&clients_head, client, path);
              }
            }
          }
        }
      }

      client = next_client;
    }
  } // infinite loop

  printf("Closing socket...\n");
  CLOSESOCKET(server);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}

SOCKET create_socket(const char *host, const char *port) {

  printf("Configure server address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(0, port, &hints, &bind_address);

  printf("Create socket...\n");
  SOCKET socket_listen =
      socket(bind_address->ai_family, bind_address->ai_socktype,
             bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen)) {
    fprintf(stderr, "socket() failed with error %d\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Bind socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    fprintf(stderr, "bind() failed with error %d\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Start listening to incoming connection...\n");
  if (listen(socket_listen, 10) < 0) {
    fprintf(stderr, "listen() failed with error %d\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  printf("\nServer listening at http://localhost:%s/\n", port);

  return socket_listen;
}

fd_set wait_on_clients(client_info *clients_head, SOCKET server) {

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(server, &readfds);
  SOCKET max_socket = server;

  client_info *client = clients_head;
  while (client) {
    FD_SET(client->socket, &readfds);
    if (client->socket > max_socket)
      max_socket = client->socket;
    client = client->next;
  }

  if (select(max_socket + 1, &readfds, 0, 0, 0) < 0) {
    fprintf(stderr, "select() failed with error %d\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  return readfds;
}

client_info *get_client(client_info **clients_head, SOCKET socket) {

  client_info *client_node = *clients_head;

  // INFO: This loop is dead code in the current version of this program where
  // this function is only called with -1. Keep it for flexibility in case you
  // run into a scenario where the server needs to look up an existing client.
  while (client_node) {
    if (client_node->socket == socket) {
      return client_node;
    }
    client_node = client_node->next;
  }

  client_node = (client_info *)calloc(1, sizeof(client_info));
  if (!client_node) {
    fprintf(stderr, "Out of memory.\n");
    exit(EXIT_FAILURE);
  }

  client_node->address_length = sizeof(struct sockaddr_storage);
  client_node->next = *clients_head;
  *clients_head = client_node;

  return client_node;
}

void send_400(client_info **clients_head, client_info *client) {
  char res_400[1024];
  char *status = "Bad Request";
  snprintf(res_400, sizeof(res_400),
           "HTTP/1.1 400 %s\r\n"
           "Connection: close\r\n"
           "Content-Length: %d\r\n\r\n"
           "%s",
           status, (int)strlen(status), status);
  send(client->socket, res_400, strlen(res_400), 0);
  drop_client(clients_head, client);
}

void send_404(client_info **clients_head, client_info *client) {
  char res_404[1024];
  char *status = "Not Found";
  snprintf(res_404, sizeof(res_404),
           "HTTP/1.1 404 %s\r\n"
           "Connection: close\r\n"
           "Content-Length: %d\r\n\r\n"
           "%s",
           status, (int)strlen(status), status);
  send(client->socket, res_404, strlen(res_404), 0);
  drop_client(clients_head, client);
}

void drop_client(client_info **clients_head, client_info *client) {
  CLOSESOCKET(client->socket);

  client_info **temp = clients_head;

  while (*temp) {
    if (*temp == client) {
      *temp = client->next;
      free(client);
      return;
    }
    temp = &(*temp)->next;
  }

  fprintf(stderr, "drop_client not found.\n");
  exit(EXIT_FAILURE);
}

const char *get_content_type(const char *full_path) {
  const char *ct;
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
      else ct = "application/octet-stream";
    // clang-format on
  } else {
    ct = "application/octet-stream";
  }

  return ct;
}
void serve_resource(client_info **clients_head, client_info *client,
                    const char *path) {
  printf("serve_resource initiated %s %s\n", client->info_buf, path);

  if (strcmp(path, "/") == 0)
    path = "/index.html";
  if (strlen(path) > 100) {
    send_400(clients_head, client);
    return;
  }
  if (strstr(path, "..")) {
    send_404(clients_head, client);
    return;
  }

  char full_path[128];
  snprintf(full_path, sizeof(full_path), "public%s", path);

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
    send_404(clients_head, client);
    return;
  }

  fseek(fp, 0L, SEEK_END);
  size_t cl = ftell(fp);
  rewind(fp);

  const char *ct = get_content_type(full_path);

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

  size_t rd;
  while ((rd = fread(buffer, 1, BSIZE, fp)) > 0)
    send(client->socket, buffer, rd, 0);
  printf("serve_resource completed %s %s\n", client->info_buf, full_path);

  fclose(fp);
  drop_client(clients_head, client);
}
