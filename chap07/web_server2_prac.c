// web_server2_prac.c

#include "chap07.h"
#define MAX_REQUEST_SIZE 2047

typedef struct client_info {
  struct sockaddr_storage address;
  socklen_t address_length;
  char request[MAX_REQUEST_SIZE + 1];
  SOCKET socket;
  int received;
  char formatted_address[128];
  struct client_info *next;
} client_info_t;

void drop_client(client_info_t **clients_head, client_info_t *client) {
  CLOSESOCKET(client->socket);

  client_info_t **next_ci = clients_head;
  while (*next_ci) {
    if (*next_ci == client) {
      *next_ci = client->next;
      free(client);
      return;
    }
    next_ci = &(*next_ci)->next;
  }

  fprintf(stderr, "drop_client failed to found.\n");
  exit(EXIT_FAILURE);
}

void send_404(client_info_t **clients_head, client_info_t *client) {
  char *content = "404 Not Found";
  size_t cl = strlen(content);
  char buffer[1024];

  snprintf(buffer, sizeof(buffer),
           "HTTP/1.1 404 %s\r\n"
           "Connection: close\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: %zu\r\n"
           "\r\n"
           "%s",
           content, cl, content);

  send(client->socket, buffer, strlen(buffer), 0);
  drop_client(clients_head, client);
}

void send_400(client_info_t **clients_head, client_info_t *client) {
  char *content = "400 Bad Request";
  size_t cl = strlen(content);
  char buffer[1024];

  snprintf(buffer, sizeof(buffer),
           "HTTP/1.1 400 %s\r\n"
           "Connection: close\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: %zu\r\n"
           "\r\n"
           "%s",
           content, cl, content);

  send(client->socket, buffer, strlen(buffer), 0);
  drop_client(clients_head, client);
}
int serve_resource(client_info_t **clients_head, client_info_t *client,
                   char *path) {
  // DEBUG
  /* printf("serve_resource initiated for %s at %s\n",
     client->formatted_address, path); */

  if (strcmp(path, "/") == 0) {
    path = "/index.html";
  }
  if (strlen(path) > 100) {
    send_400(clients_head, client);
    return EXIT_SUCCESS;
  }
  if (strstr(path, "..")) {
    send_404(clients_head, client);
    return EXIT_SUCCESS;
  }

  char full_path[128];
  snprintf(full_path, sizeof(full_path), "public%s", path);
#ifdef _WIN32
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
    return EXIT_SUCCESS;
  }

  char *ct = NULL;
  char *last_dot = strrchr(full_path, '.');
  if (last_dot) {
    // clang-format off
    if      (strcmp(last_dot, ".html") == 0) ct = "text/html";
    else if (strcmp(last_dot, ".css")  == 0) ct = "text/css";
    else if (strcmp(last_dot, ".csv")  == 0) ct = "text/csv";
    else if (strcmp(last_dot, ".txt")  == 0) ct = "text/plain";
    else if (strcmp(last_dot, ".js")   == 0) ct = "application/javascript";
    else if (strcmp(last_dot, ".json") == 0) ct = "application/json";
    else if (strcmp(last_dot, ".pdf")  == 0) ct = "application/pdf";
    else if (strcmp(last_dot, ".gif")  == 0) ct = "image/gif";
    else if (strcmp(last_dot, ".ico")  == 0) ct = "image/x-icon";
    else if (strcmp(last_dot, ".jpeg") == 0) ct = "image/jpeg";
    else if (strcmp(last_dot, ".jpg")  == 0) ct = "image/jpeg";
    else if (strcmp(last_dot, ".png")  == 0) ct = "image/png";
    else if (strcmp(last_dot, ".svg")  == 0) ct = "image/svg+xml";
    else                                     ct = "application/octet-stream";
    // clang-format on
  } else {
    ct = "application/octet-stream";
  }

  fseek(fp, 0, SEEK_END);
  size_t cl = ftell(fp);
  rewind(fp);

#define RES_BUF 1024
  char buffer[RES_BUF];

  snprintf(buffer, RES_BUF, "HTTP/1.1 200 Ok\r\n");
  send(client->socket, buffer, strlen(buffer), 0);
  snprintf(buffer, RES_BUF, "Connection: close\r\n");
  send(client->socket, buffer, strlen(buffer), 0);
  snprintf(buffer, RES_BUF, "Content-Type: %s\r\n", ct);
  send(client->socket, buffer, strlen(buffer), 0);
  snprintf(buffer, RES_BUF, "Content-Length: %zu\r\n", cl);
  send(client->socket, buffer, strlen(buffer), 0);
  snprintf(buffer, RES_BUF, "\r\n");
  send(client->socket, buffer, strlen(buffer), 0);

  int b8_read;
  while ((b8_read = fread(buffer, sizeof(char), RES_BUF, fp)) > 0) {
    send(client->socket, buffer, b8_read, 0);
  }

  printf("serve_resource completed for %s at %s\n", client->formatted_address,
         full_path);

  fclose(fp);
  drop_client(clients_head, client);

  return EXIT_SUCCESS;
}

int main(void) {

#ifdef _WIN32
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WSAStartup(wVersionRequested, &WSAData);
  if (!wsaerr) {
    printf("The dll found.\n");
    printf("Winsock Status: %s\n", WSAData.szSystemStatus);
  } else {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  char *port = "8080";

  // Create socket
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(0, port, &hints, &bind_address);

  printf("Creating socket...\n");
  SOCKET server;
  server = socket(bind_address->ai_family, bind_address->ai_socktype,
                  bind_address->ai_protocol);
  if (!ISVALIDSOCKET(server)) {
    perror("socket() failed");
    exit(EXIT_FAILURE);
  }

  printf("Binding socket to local address...\n");
  if (bind(server, bind_address->ai_addr, bind_address->ai_addrlen)) {
    perror("bind() failed");
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(bind_address);

  printf("Listening ... ");
  if (listen(server, 10) < 0) {
    perror("listen() failed");
    exit(EXIT_FAILURE);
  }
  printf("on port %s\n", port);

  client_info_t *clients_head = NULL;
  // Waiting for incoming connection to service in a loop
  while (1) {

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server, &readfds);
    SOCKET max_socket = server;

    client_info_t *meta_client = clients_head;
    while (meta_client) {
      FD_SET(meta_client->socket, &readfds);
      if (meta_client->socket > max_socket)
        max_socket = meta_client->socket;
      meta_client = meta_client->next;
    }

    if (select(max_socket + 1, &readfds, NULL, NULL, NULL) < 0) {
      perror("select() failed");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(server, &readfds)) {
      // Get new client_info_t
      client_info_t *client = NULL;
      {
        // For maintenance use case param is: SOCKET socket;
        SOCKET socket = -1;

        client_info_t *ci = NULL;
        ci = clients_head;
        while (ci) {
          if (ci->socket == socket)
            client = ci;
          ci = ci->next;
        }

        if (!ci) {
          ci = (client_info_t *)calloc(1, sizeof(client_info_t));
          if (!ci) {
            fprintf(stderr, "Memory allocation failed.\n");
            exit(EXIT_FAILURE);
          }
          ci->address_length = sizeof(ci->address);
          ci->next = clients_head;
          clients_head = ci;

          client = ci;
        }
      }
      // Accept new incoming connection
      client->socket = accept(server, (struct sockaddr *)&client->address,
                              &client->address_length);
      if (!ISVALIDSOCKET(client->socket)) {
        perror("accept() failed");
        exit(EXIT_FAILURE);
      }
      // Cache new client address and host
      char address_buffer[64];
      char service_buffer[64];
      getnameinfo((struct sockaddr *)&client->address, client->address_length,
                  address_buffer, sizeof(address_buffer), service_buffer,
                  sizeof(service_buffer), NI_NUMERICHOST | NI_NUMERICSERV);
      snprintf(client->formatted_address, 128, "%s:%s", address_buffer,
               service_buffer);
      // Print new client address and host
      printf("New connection from: %s\n", client->formatted_address);
    }

    client_info_t *ptr_ci = clients_head;
    while (ptr_ci) {
      client_info_t *next = ptr_ci->next;

      if (FD_ISSET(ptr_ci->socket, &readfds)) {
        if (ptr_ci->received == MAX_REQUEST_SIZE) {
          send_400(&clients_head, ptr_ci);
          ptr_ci = next;
          continue;
        }

        int b8_rcvd = recv(ptr_ci->socket, ptr_ci->request + ptr_ci->received,
                           MAX_REQUEST_SIZE - ptr_ci->received, 0);
        if (b8_rcvd < 1) {
          printf("Unexpected disconnect from: %s\n", ptr_ci->formatted_address);
          drop_client(&clients_head, ptr_ci);

        } else {
          ptr_ci->received += b8_rcvd;
          ptr_ci->request[ptr_ci->received] = 0;

          char *meta = strstr(ptr_ci->request, "\r\n\r\n");
          if (meta) {
            *meta = 0;
            if (strncmp(ptr_ci->request, "GET /", 5)) {
              send_400(&clients_head, ptr_ci);

            } else {
              char *path = ptr_ci->request + 4;
              char *end_path = strstr(path, " ");
              if (!end_path) {
                send_400(&clients_head, ptr_ci);

              } else {
                *end_path = 0;
                serve_resource(&clients_head, ptr_ci, path);
              }
            }
          }
        }
      }

      ptr_ci = next;
    }
  }

  printf("\nClosing socket...\n");
  CLOSESOCKET(server);

#ifdef _WIN32
  WSACleanup();
#endif

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
