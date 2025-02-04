/* 06_web_get.c */

#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void parse_url(char *url, char **hostname, char **port, char **path) {
  printf("\n%8s: %s\n", "url", url);

  char *separator = "://";
  char *ptr = strstr(url, separator);

  if (ptr) {
    char *protocol = url;
    *ptr = 0;
    if (strcmp(protocol, "http")) {
      fprintf(stderr, "Unknown protocol '%s'. Only 'http' is supported.\n",
              protocol);
      exit(EXIT_FAILURE);
    }
    ptr += strlen(separator);
  } else {
    ptr = url;
  }

  *hostname = ptr;
  while (*ptr && (*ptr != ':') && (*ptr != '/') && (*ptr != '#'))
    ptr++;

  *port = "80";
  if (*ptr == ':') {
    *ptr = 0;
    *port = ++ptr;
  }

  while (*ptr && (*ptr != '/') && (*ptr != '#'))
    ptr++;

  *path = ptr;
  if (*ptr == '/')
    *path = ptr + 1;
  *ptr++ = 0;

  while (*ptr && (*ptr != '#'))
    ptr++;

  if (*ptr == '#')
    *ptr = 0;

  printf("%8s: %s\n", "hostname", *hostname);
  printf("%8s: %s\n", "port", *port);
  printf("%8s: %s\n", "path", *path);
  printf("\n");
}

int create_socket(const char *host, const char *port) {
  printf("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *peer_address;
  int gai_err = getaddrinfo(host, port, &hints, &peer_address);
  if (gai_err) {
    fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(gai_err));
    exit(EXIT_FAILURE);
  }

  printf("Resolved remote server: ");
  char address_buffer[64];
  char service_buffer[64];
  int gni_err =
      getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen,
                  address_buffer, sizeof(address_buffer), service_buffer,
                  sizeof(service_buffer), NI_NUMERICHOST);
  if (gni_err) {
    fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(gni_err));
    exit(EXIT_FAILURE);
  }
  printf("%s:%s\n", address_buffer, service_buffer);

  printf("Creating socket...\n");
  int server = socket(peer_address->ai_family, peer_address->ai_socktype,
                      peer_address->ai_protocol);
  if (server < 0) {
    perror("socket() failed");
    exit(EXIT_FAILURE);
  }

  printf("Connecting to remote server...\n");
  if (connect(server, peer_address->ai_addr, peer_address->ai_addrlen)) {
    perror("connect() failed");
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(peer_address);

  printf("Connected.\n");
  return server;
}

void send_request(int server, const char *host, const char *port,
                  const char *path) {
#define MX_RQ_S 2048
  char buffer[MX_RQ_S];
  int len = 0;

  len += snprintf(buffer + len, MX_RQ_S - len, "GET /%s HTTP/1.1\r\n", path);
  len += snprintf(buffer + len, MX_RQ_S - len, "Host: %s:%s\r\n", host, port);
  len += snprintf(buffer + len, MX_RQ_S - len,
                  "User-Agent: honwpc weg_get 1.3.0\r\n");
  len += snprintf(buffer + len, MX_RQ_S - len, "Connection: close\r\n");
  len += snprintf(buffer + len, MX_RQ_S - len, "\r\n");

  if (len >= MX_RQ_S) {
    fprintf(stderr, "Request exceeded buffer size.\n");
    return;
  }

  if (send(server, buffer, strlen(buffer), 0) < 0)
    perror("send() failed");

  printf("\nSent headers:\n%s", buffer);
}

int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(stderr, "Usage: web_get url\n");
    exit(EXIT_FAILURE);
  }

  // Parse the url
  char *host = NULL, *port = NULL, *path = NULL;
  parse_url(argv[1], &host, &port, &path);

  // Create the socket
  int server = create_socket(host, port);

  // Send the request
  send_request(server, host, port, path);
  // Start count down
  clock_t start_time = clock();

// Manage TCP packets and parse the response
// Declare bookeeping variables
#define TIMEOUT 5.0
#define RES_BUF_LEN (32 * 1024 - 1)

  char *response = (char *)calloc(RES_BUF_LEN + 1, sizeof(char));
  if (!response) {
    perror("calloc() failed");
    exit(EXIT_FAILURE);
  }
  char *end = response + RES_BUF_LEN;
  char *pkt = response;
  char *body = NULL;
  char *meta = NULL;
  enum { length, chunked, connection };
  int encoding = length;
  int remaining = 0;

  while (1) {
    if ((double)(clock() - start_time) / CLOCKS_PER_SEC > TIMEOUT) {
      fprintf(stderr, "Timeout after %.2f\n", TIMEOUT);
      exit(EXIT_FAILURE);
    }
    if (pkt == end) {
      fprintf(stderr, "Out of buffer space.\n");
      exit(EXIT_FAILURE);
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    if (select(server + 1, &readfds, NULL, NULL, &timeout) < 0) {
      perror("select() failed");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(server, &readfds)) {
      int b8_rcvd = recv(server, pkt, end - pkt, 0);
      if (b8_rcvd < 1) {
        if (encoding == connection && body) {
          printf("%.*s\n", (int)(end - body), body);
        }
        printf("\nConnection closed by peer.\n");
        break;
      }

      // TEST: For debugging, watch TCP  packet splitting
      printf("\nReceived Data (%d bytes)\n->>%.*s<<-\n", b8_rcvd, b8_rcvd, pkt);

      pkt += b8_rcvd;
      *pkt = 0;

      char *headers_end = "\r\n\r\n";
      if (!body && (meta = strstr(response, headers_end))) {
        *meta = 0;
        body = meta + strlen(headers_end);
        printf("\nReceived headers:\n%s\n", response);

        char *length_hdr = "\nContent-Length:";
        char *chunked_hdr = "\nTransfer-Encoding: chunked";

        if ((meta = strstr(response, length_hdr))) {
          encoding = length;
          meta += strlen(length_hdr);
          remaining = strtol(meta, NULL, 10);

        } else if ((meta = strstr(response, chunked_hdr))) {
          encoding = chunked;
          remaining = 0;

        } else {
          encoding = connection;
        }

        printf("\nReceived body:\n");
      }

      if (body) {
        if (encoding == length) {
          if (pkt - body >= remaining) {
            printf("%.*s", remaining, body);
            break; // break out of while(1)
          }
        } else if (encoding == chunked) {
          do {
            if (remaining == 0) {
              if ((meta = strstr(body, "\r\n"))) {
                remaining = strtol(body, NULL, 16);
                if (!remaining)
                  goto finish;
                body = meta + 2;
              } else
                break;
            }
            if (remaining && pkt - body >= remaining) {
              printf("%.*s", remaining, body);
              body += remaining + 2;
              remaining = 0;
            }
          } while (!remaining);
        }
      }
    }
  } // while(1)
finish:

  // Cleanup routines
  free(response);
  // Close the socket
  printf("\n\nClosing socket...\n");
  close(server);

  printf("Finished.\n");
  return EXIT_SUCCESS;
}
