// web_server_prac.c

#include "chap07.h"

#define MAX_REQUEST_SIZE 2047
// Struct to store information about each connected client in linked list
typedef struct client_info {
  struct sockaddr_storage address;
  socklen_t address_length;
  SOCKET socket;
  char request[MAX_REQUEST_SIZE + 1];
  int received;
  struct client_info *next;
} client_info_t;
// Make the head of the linked list globally available
static client_info_t *clients = NULL;

// Helper functions prototypes
SOCKET create_socket(const char *host, const char *port);
fd_set wait_on_clients(SOCKET server);
const char *get_client_address(client_info_t *client);
client_info_t *get_client(SOCKET socket);
void send_400(client_info_t *client);
void send_404(client_info_t *client);
void drop_client(client_info_t *client);
const char *get_content_type(const char *path);
void serve_resource(client_info_t *client, const char *path);

int main(void) {

#ifdef _WIN32
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WSAStartup(wVersionRequested, &WSAData);
  if (!wsaerr) {
    printf("The dll found.\n");
    printf("Winsock Status: %s\n", WSAData.szSystemStatus);
  } else {
    fprintf(stderr, "The dll not found. Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  // Create socket:
  SOCKET server = create_socket(0, "8080");

  while (1) {

    // Create fd set and call select() to block while waiting for socket events
    fd_set readfds = wait_on_clients(server);

    if (FD_ISSET(server, &readfds)) {
      client_info_t *client = get_client(-1);

      client->socket = accept(server, (struct sockaddr *)&client->address,
                              &client->address_length);
      if (!ISVALIDSOCKET(client->socket)) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
      }

      printf("New connection from: %s\n", get_client_address(client));
    }

    client_info_t *client = clients;
    while (client) {
      client_info_t *next_client = client->next;

      if (FD_ISSET(client->socket, &readfds)) {
        if (client->received == MAX_REQUEST_SIZE) {
          send_400(client);
          client = next_client;
          continue;
        }

        int b8_rcvd = recv(client->socket, client->request + client->received,
                           MAX_REQUEST_SIZE - client->received, 0);
        if (b8_rcvd < 1) {
          printf("Unexpected disconnect from %s\n", get_client_address(client));
          drop_client(client);

        } else {
          client->received += b8_rcvd;
          client->request[client->received] = 0;

          char *meta = strstr(client->request, "\r\n\r\n");
          if (meta) {
            *meta = '\0';
            char *supported_type = "GET /";
            if (strncmp(supported_type, client->request, 5)) {
              send_400(client);
            } else {
              char *path = client->request + (strlen(supported_type) - 1);
              char *path_end = strstr(path, " ");
              if (!path_end) {
                send_400(client);
              } else {
                *path_end = 0;
                serve_resource(client, path);
              }
            } // whether GET request or not
          } // if headers properly delimited
        } // whether b8_rcvd < 1 or not
      } // if client socket is in readfds

      client = next_client;
    } // loop all clients
  } // while(1)

  printf("Closing socket...\n");
  CLOSESOCKET(server);

#ifdef _WIN32
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}

SOCKET create_socket(const char *host, const char *port) {

  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(host, port, &hints, &bind_address);

  printf("Creating socket...\n");
  SOCKET socket_listen;
  socket_listen = socket(bind_address->ai_family, bind_address->ai_socktype,
                         bind_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen)) {
    perror("socket() failed");
    exit(EXIT_FAILURE);
  }

  printf("Binding socket to local address...\n");
  if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen)) {
    perror("bind() failed");
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Listening... ");
  if (listen(socket_listen, 10) < 0) {
    perror("listen() failed");
    exit(EXIT_FAILURE);
  }
  printf("on port '%s'\n", port);

  return socket_listen;
}

fd_set wait_on_clients(SOCKET server) {

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(server, &readfds);
  SOCKET max_socket = server;

  client_info_t *client = clients;
  while (client) {
    FD_SET(client->socket, &readfds);
    if (client->socket > max_socket)
      max_socket = client->socket;
    client = client->next;
  }

  if (select(max_socket + 1, &readfds, NULL, NULL, 0) < 0) {
    perror("select() failed");
    exit(EXIT_FAILURE);
  }

  return readfds;
}

const char *get_client_address(client_info_t *client) {
  static char p_formatted_address[200];
  char p_address_buffer[100];
  char p_service_buffer[100];
  getnameinfo((struct sockaddr *)&client->address, client->address_length,
              p_address_buffer, sizeof(p_address_buffer), p_service_buffer,
              sizeof(p_service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
  snprintf(p_formatted_address, 200, "%s:%s", p_address_buffer,
           p_service_buffer);
  return p_formatted_address;
}

client_info_t *get_client(SOCKET socket) {
  client_info_t *p_client = clients;

  while (p_client) {
    if (p_client->socket == socket)
      return p_client;
    p_client = p_client->next;
  }

  p_client = (client_info_t *)calloc(1, sizeof(client_info_t));
  if (p_client == NULL) {
    fprintf(stderr, "Memory allocation error.\n");
    exit(EXIT_FAILURE);
  }
  p_client->address_length = sizeof(p_client->address);
  p_client->next = clients;
  clients = p_client;

  return p_client;
}

void send_400(client_info_t *client) {
  char *content = "Bad Request";
  char r400[2048];
  snprintf(r400, sizeof(r400),
           "HTTP/1.1 400 %s\r\n"
           "Connection: close\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: %lu\r\n"
           "\r\n"
           "\r\n"
           "%s",
           content, strlen(content), content);

  send(client->socket, r400, strlen(r400), 0);
  drop_client(client);
}

void send_404(client_info_t *client) {
  char *content = "Not Found";
  char r404[2048];
  snprintf(r404, sizeof(r404),
           "HTTP/1.1 404 %s\r\n"
           "Connection: close\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: %lu\r\n"
           "\r\n"
           "\r\n"
           "%s",
           content, strlen(content), content);

  send(client->socket, r404, strlen(r404), 0);
  drop_client(client);
}

void drop_client(client_info_t *client) {
  CLOSESOCKET(client->socket);

  client_info_t **ptr = &clients;
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

void serve_resource(client_info_t *client, const char *path) {
  // DEBUG
  // printf("serve_resource initiated: %s %s\n", get_client_address(client), path);

  if (strcmp(path, "/") == 0) {
    path = "/index.html";
  }
  if (strlen(path) > 100) {
    send_400(client);
    return;
  }
  if (strstr(path, "..")) {
    send_404(client);
    return;
  }

  char fullpath[128];
  snprintf(fullpath, sizeof(fullpath), "public%s", path);
#ifdef _WIN32
  char *p = fullpath;
  while (*p) {
    if (*p == '/')
      *p = '\\';
    p++;
  }
#endif

  FILE *fp = fopen(fullpath, "rb");
  if (!fp) {
    send_404(client);
    return;
  }

  fseek(fp, 0L, SEEK_END);
  size_t cl = ftell(fp);
  rewind(fp);

  const char *ct = get_content_type(fullpath);

#define RESP_BUF_SIZE 1024
  char resp_buf[RESP_BUF_SIZE];

  snprintf(resp_buf, RESP_BUF_SIZE, "HTTP/1.1 200 Ok\r\n");
  send(client->socket, resp_buf, strlen(resp_buf), 0);
  snprintf(resp_buf, RESP_BUF_SIZE, "Connection: close\r\n");
  send(client->socket, resp_buf, strlen(resp_buf), 0);
  snprintf(resp_buf, RESP_BUF_SIZE, "Content-Length: %zu\r\n", cl);
  send(client->socket, resp_buf, strlen(resp_buf), 0);
  snprintf(resp_buf, RESP_BUF_SIZE, "Content-Type: %s\r\n", ct);
  send(client->socket, resp_buf, strlen(resp_buf), 0);
  snprintf(resp_buf, RESP_BUF_SIZE, "\r\n");
  send(client->socket, resp_buf, strlen(resp_buf), 0);

  size_t b8_read;
  while ((b8_read = fread(resp_buf, sizeof(char), RESP_BUF_SIZE, fp)) > 0) {
    send(client->socket, resp_buf, b8_read, 0);
  }

  // DEBUG
  printf("serve_resource completed: %s %s\n", get_client_address(client),
         fullpath);

  fclose(fp);
  drop_client(client);
}
