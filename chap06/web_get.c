/* web_get.c */

#include "chap06.h"

#define TIMEOUT 5.0

void parse_url(char *url, char **hostname, char **port, char **path);
void send_request(SOCKET s, char *hostname, char *port, char *path);
SOCKET connect_to_host(char *hostname, char *port);

/**
 * @brief This function implement an HTTP web client.
 *
 * This client takes as input as URL. It then attempts to connect to the
 * host and retrieve the resource given by the URL. The program displays the
 * HTTP headers that are sent and received, and it attempts to parse out the
 * requested resource content from the HTTP response.
 * */
int main(int argc, char *argv[]) {

#ifdef _WIN32
  WSADATA WSAData;
  int wVersionRequested = MAKEWORD(2, 2);
  unsigned int wsaerr = WSAStartup(wVersionRequested, &WSAData);
  if (wsaerr) {
    fprintf(stderr, "Failed to initialize Winsock. The dll not found.\n");
    exit(EXIT_SUCCESS);
  } else {
    printf("The dll found.\n");
    printf("Winsock Status: %s.\n", WSAData.szSystemStatus);
  }
#endif

  if (argc < 2) {
    fprintf(stderr, "usage: web_get url\n");
    return EXIT_FAILURE;
  }
  char *url = argv[1];

  // Parse url
  char *hostname, *port, *path;
  parse_url(url, &hostname, &port, &path);
  // Establish connection and send HTTP request
  SOCKET server = connect_to_host(hostname, port);
  send_request(server, hostname, port, path);

  // Set timeout start
  const clock_t start_time = clock();
// Define variables for bookkeeping while receiving and parsing HTTP response.
/* #define RESPONSE_8K (8 * 1024) */
#define RESPONSE_32K (32 * 1024)
  /* Statically point to the beginning of the buffer the server response is
   * recursively read into by split TCP packet (over server chunks). */
  char *response = (char *)calloc((RESPONSE_32K + 1), sizeof(char));
  if (!response) {
    perror("Memory allocation failed.");
    exit(EXIT_FAILURE);
  }
  /* Dynamically tracks how far TCP split packets, from recv(), have been
   * concatenated into the response buffer. */
  char *pkt = response;
  /* Helper pointer to locate critical meta-data within each packet and
   * dynamically update the position of the remaining body chunks to read */
  char *meta = NULL;
  /* Statically point to the last unused byte in the response buffer */
  char *end = response + RESPONSE_32K;
  /* Dynamically tracks the changing position to read the response body, once
   * per chunk or length */
  char *body = NULL;
  /* If you recall, the HTTP response body length can be determined by a few
   * different methods. We define an enumeration to list the method types, and
   * we define the "encoding" variable to store the actual method used. */
  typedef enum { length, chunked, connectionClosed } BodyLenghtEncoding;
  BodyLenghtEncoding encoding = 0;
  /* Records how many bytes are still needed to finish the HTTP body or body
   * chunk */
  int remaining = 0;

  // Start the loop to receive and process HTTP response
  while (1) {
    if ((clock() - start_time) / (double)CLOCKS_PER_SEC > TIMEOUT) {
      fprintf(stderr, "timeout after %.2f seconds\n", TIMEOUT);
      return EXIT_FAILURE;
    }
    if (pkt == end) {
      fprintf(stderr, "out of buffer space\n");
      return EXIT_FAILURE;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;       // holds seconds
    timeout.tv_usec = 200000; // holds microseconds

    if (select(server + 1, &readfds, 0, 0, &timeout) < 0) {
      fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
      return EXIT_FAILURE;
    }

    // Read in new data and detect closed connection
    if (FD_ISSET(server, &readfds)) {
      int b8_rcvd = recv(server, pkt, end - pkt, 0);
      if (b8_rcvd < 1) {
        if (encoding == connectionClosed && body) {
          printf("%.*s", (int)(end - body), body);
        }
        /* Actually the next statement is a lame justification to the fact that
         * this client is the one who intend on shutting down the connection
         * because it doesn't support body length indicator other than
         * 'Content-Length' and 'Transfer-Encoding', consequently, as a result
         * of the 'Connection: close' request header the server will immediately
         * close the connection. */
        printf("\nConnection closed by peer.\n");
        break;
      }

      /* TEST: Toggle off the next printf() call to have a taste of TCP packets
       * splitting on top of chunked data. TCP is a stream-oriented protocol,
       * which means it guarantees that data arrives in order and without loss,
       * but it doesn’t guarantee how much data will be delivered in each recv()
       * call. If a large message is sent, it might be split into multiple
       * packets or be delivered all at once, depending on network conditions,
       * buffer sizes, and other factors. But the full message will eventually
       * arrive intact and in the correct order. */
      printf("\nReceived Data (%d bytes) ->>%.*s<<-\n", b8_rcvd, b8_rcvd, pkt);

      pkt += b8_rcvd;
      *pkt = 0;

      // Search for the end of the HTTP headers or beginning of HTTP body
      char *headers_end = "\r\n\r\n";
      if (!body && (meta = strstr(response, headers_end))) {
        *meta = 0;
        body = meta + strlen(headers_end);
        printf("\nReceived Headers:\n%s\n", response);

        // Determine which body length method is used.
        char *content_length = "\nContent-Length:";
        meta = strstr(response, content_length);
        if (meta) {
          encoding = length;
          meta += strlen(content_length);
          remaining = strtol(meta, 0, 10);

        } else {
          meta = strstr(response, "\nTransfer-Encoding: chunked");
          if (meta) {
            encoding = chunked;
            remaining = 0;
          } else {
            /* If the server doesn't send either way of indicating body length,
             * then we assume that the entire HTTP body has been received once
             * the connection is closed without further parsing as warned in the
             * request Connection header. */
            encoding = connectionClosed;
          }
        }
        printf("\nReceived Body:\n");

      } // if (!body && (body = strstr(response, "\r\n\r\n")))

      if (body) {
        if (encoding == length) {
          if (pkt - body >= remaining) {
            printf("%.*s", remaining, body);
            break;
          }

        } else if (encoding == chunked) {
          do {
            if (remaining == 0) {
              if ((meta = strstr(body, "\r\n"))) {
                remaining = strtol(body, 0, 16);
                if (!remaining)
                  goto finish;
                body = meta + 2;
              } else
                /* Then the received chunk doesn't have any line with hexa
                 * number and we need to break out to call recv() again */
                break;
            }
            if (remaining && pkt - body >= remaining) {
              printf("%.*s", remaining, body);
              body += remaining + 2;
              remaining = 0;
            }
          } while (!remaining); // if not go call recv() again
        } // else if (encoding == chunked)
      } // if (body)
    } // if (FD_ISSET(server, &readfds))
  } // while(1)
finish:

  // Cleanup routines
  free(response);
  printf("\nClosing socket...\n");
  CLOSESOCKET(server);

#if defined(_WIN32)
  WSACleanup();
#endif

  printf("Finished\n");
  return EXIT_SUCCESS;
}

/**
 * @brief A function to parse a given URL.
 *
 * The function takes as input as URL, and it returns as output the hostname,
 * the port number, and the document path. To avoid needing to do manual memory
 * management, the outputs are returned as pointers to specific parts of the
 * input URL. The input URL is modified by null-terminating specific positions.
 *
 * @param url The URL to parse.
 * @param hostname A pointer to the start of the hostname.
 * @param port A pointer to the start of the port number (or 0 if not
 * specified).
 * @param path A pointer to the start of the document path.
 */
void parse_url(char *url, char **hostname, char **port, char **path) {
  // For debugging purposes, optionally printf the URL.
  printf("%8s:\t%s\n", "URL", url);

  // parse the protocol
  char *scheme_separator = "://";
  char *p;
  p = strstr(url, scheme_separator);
  char *protocol = 0;
  if (p) {
    protocol = url;
    *p = 0;
    p += strlen(scheme_separator);
  } else {
    p = url;
  }

  if (protocol) {
    if (strcmp(protocol, "http")) {
      fprintf(stderr, "Unknown protocol '%s'. Only 'http' is supported.\n",
              protocol);
      exit(EXIT_FAILURE);
    }
  }

  // Return the hostname
  *hostname = p;
  while (*p && *p != ':' && *p != '/' && *p != '#')
    ++p;

  // If set return the port number or return 80
  *port = "80";
  if (*p == ':') {
    *p++ = 0;
    *port = p;
  }
  while (*p && *p != '/' && *p != '#')
    ++p;

  // Set the path variable: Since we must null-terminate after the hostname and
  // port (if specified) by replacing ':' or '/' with '\0', unless we want to
  // indulge in memory allocation, simplicity asks to skip it. All document
  // paths start with '/', so the function caller can easily prepend that when
  // the HTTP request is constructed.
  *path = p;
  if (*p == '/')
    *path = p + 1;
  *p++ = 0;

  // Check for hash and ignore it since it is never sent to the web server
  while (*p && *p != '#')
    ++p;
  if (*p == '#')
    *p = 0;

  // Print out returned values for debugging purposes
  printf("%8s:\t%s\n", "Hostname", *hostname);
  printf("%8s:\t%s\n", "Port", *port);
  printf("%8s:\t%s\n\n", "Path", *path);
}

/**
 * @brief A function to connect to a remote host and return the socket.
 *
 * This function takes a hostname and port number and attempts to connect to
 * the remote host using a TCP socket. If the connection is successful, the
 * socket is returned; otherwise, the program exits with an error message.
 *
 * @param hostname The hostname or IP address of the remote host.
 * @param port The port number of the remote host.
 * @return The socket used to connect to the remote host.
 */
SOCKET connect_to_host(char *hostname, char *port) {
  // Configure remote address
  printf("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo *peer_address;
  if (getaddrinfo(hostname, port, &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  // Print hostname resolution result for debugging purposes
  printf("Remote address is: ");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST);
  printf("%s %s\n", address_buffer, service_buffer);

  // Create a new socket to establish the TCP connection
  printf("Creating socket...\n");
  SOCKET server;
  server = socket(peer_address->ai_family, peer_address->ai_socktype,
                  peer_address->ai_protocol);
  if (!ISVALIDSOCKET(server)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Connecting...\n");
  if (connect(server, peer_address->ai_addr, peer_address->ai_addrlen)) {
    fprintf(stderr, "connect() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(peer_address);

  printf("Connected.\n\n");

  return server;
}

/**
 * @brief A function to send a GET request to a server.
 *
 * This function sends a GET request through the specified socket to the
 * specified server. The hostname and port number are used to construct the
 * Host header. The User-Agent header is set to "honpwc web_get 1.0". The
 * request body is empty.
 *
 * @param s Socket to send the request through.
 * @param hostname The hostname of the server.
 * @param port The port number of the server.
 * @param path The document path for the GET request.
 */
void send_request(SOCKET server, char *hostname, char *port, char *path) {
  // Construct the HTTP request. GET request only needs headers.
  char buffer[2048];
  sprintf(buffer, "GET /%s HTTP/1.1\r\n", path);
  sprintf(buffer + strlen(buffer), "Host: %s:%s\r\n", hostname, port);
  sprintf(buffer + strlen(buffer), "Connection: close\r\n");
  // This User-Agent stands for: Book_Title This_Program_Title Version
  sprintf(buffer + strlen(buffer), "User-Agent: honpwc web_get 1.0\r\n");
  sprintf(buffer + strlen(buffer), "\r\n");

  // Send the request
  send(server, buffer, strlen(buffer), 0);

  // Print out the request for debugging purposes
  printf("Sent Headers:\n%s", buffer);
}
