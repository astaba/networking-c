/* smtp_send.c */
/* WARN: 220 server response is unlikely to come, due to the plethora of actions
 * taken by service providers to curb Spam plagues across the net.
 * For example:
 * Many residential ISPs don't allow outgoing connections on port 25. If your
 * residential provider blocks port 25, then you won't be able to establish an
 * SMTP connection. In this case, you may consider renting a virtual private
 * server to run this chapter's code.
 * Even if your ISP does allow outgoing connections on port 25, many SMTP
 * servers won't accept mail from a residential IP address. Of the servers that
 * do, many will send those emails straight into a spam folder.
 * */

#include "chap08.h"

#define MAXINPUT 512
#define MAXRESPONSE 1024

void get_input(const char *promt, char *buffer);
void send_format(SOCKET server, const char *text, ...);
int parse_response(const char *response);
void wait_on_response(SOCKET server, int expecting);
SOCKET connect_to_host(const char *hostname, const char *port);

/**
 * @brief Implements a simple SMTP client.
 *
 * This function implements a simple SMTP client according to RFC 5321.
 * It connects to a specified SMTP server, sends a HELO command, specifies
 * the sender and recipient, and sends a short email message.
 *
 * @return 0 on success, EXIT_FAILURE on error.
 *
 * @note This function blocks until the user has entered all the required
 * information and sent the email.
 */
int main(void) {

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  // Get the SMTP hotname
  char hostname[MAXINPUT];
  get_input("mail server: ", hostname);

  // Establish connection to server default service
  printf("Connecting to host: %s:25\n", hostname);
  SOCKET server = connect_to_host(hostname, "25");

  // Then wait for 220 response before issuing any commands
  wait_on_response(server, 220);

  // Follow the "HELO" command with client hostname.
  send_format(server, "HELO HONPWC\r\n");
  wait_on_response(server, 250);

  char sender[MAXINPUT];
  get_input("from: ", sender);
  send_format(server, "MAIL FROM:<%s>\r\n", sender);
  wait_on_response(server, 250);

  char recipient[MAXINPUT];
  get_input("to: ", recipient);
  send_format(server, "RCPT TO:<%s>\r\n", recipient);
  wait_on_response(server, 250);

  send_format(server, "DATA\r\n");
  wait_on_response(server, 354);

  char subject[MAXINPUT];
  get_input("subject: ", subject);

  send_format(server, "From:<%s>\r\n", sender);
  send_format(server, "To:<%s>\r\n", recipient);
  send_format(server, "Subject:<%s>\r\n", subject);

  time_t timer;
  time(&timer);
  struct tm *timeinfo;
  timeinfo = gmtime(&timer);
  char date[128];
  strftime(date, 128, "%a, %d %b %Y %H:%M:%S +0000", timeinfo);

  send_format(server, "Date:%s\r\n", date);

  send_format(server, "\r\n");

  printf("Enter your email text, ending with \".\" on a line by itself.\n");
  while (1) {
    char body[MAXINPUT];
    get_input("> ", body);
    send_format(server, "%s\r\n", body);
    if (strcmp(body, ".") == 0)
      break;
  }

  wait_on_response(server, 250);

  send_format(server, "QUIT\r\n");
  wait_on_response(server, 221);

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
 * Prompts the user for input and stores the result in the buffer.
 *
 * @param promt The prompt to display.
 * @param buffer The buffer to store the input in. It must be at least
 *               MAXINPUT bytes big.
 *
 * The function will exit if it is unable to read the input.
 */
void get_input(const char *promt, char *buffer) {
  printf("%s\n", promt);

  if (fgets(buffer, MAXINPUT, stdin) != NULL) {
    // Remove newline to emulate gets().
    buffer[strcspn(buffer, "\n")] = '\0';
  } else {
    perror("Unable to read input");
    exit(EXIT_FAILURE);
  }
}

/**
 * Send a formatted string to the server, and print the sent data to stdout.
 *
 * @param server The socket to send the data on.
 * @param text The format string. It will be passed to vsnprintf() to format
 *             the data.
 * @param ... The arguments to vsnprintf(). They will be used to fill the format
 *            string.
 *
 * The function will exit if it is unable to send the data.
 */
void send_format(SOCKET server, const char *text, ...) {
  char buffer[1024];
  va_list args;
  va_start(args, text);
  vsnprintf(buffer, sizeof(buffer), text, args);

  send(server, buffer, strlen(buffer), 0);
  // C: Client
  printf("C: %s\n", buffer);
}

/**
 * Parse a SMTP response string and return the 3 digit response code.
 *
 * The function will scan the string until a 3 digit number is found at the
 * start of a line. If the 3 digit number is followed by a '-' it is
 * considered to be a multi-line response and the function will return 0.
 *
 * @param response The string to parse.
 *
 * @return The 3 digit response code, or 0 if the response does not contain a
 *         3 digit code.
 */
int parse_response(const char *response) {
  const char *k = response;
  if (!k[0] || !k[1] || !k[2])
    return 0;

  for (; k[3]; k++) {
    if (k == response || k[-1] == '\n') {
      if (isdigit(k[0]) && isdigit(k[1]) && isdigit(k[2])) {
        if (k[3] != '-') {
          if (strstr(k, "\r\n"))
            return strtol(k, 0, 10);
        }
      }
    }
  }
  return 0;
}

/**
 * Waits for a specific SMTP response code from the server.
 *
 * Continuously receives data from the server socket until a valid
 * 3-digit SMTP response code is detected within the received data.
 * If the response code matches the expected code, the function
 * completes successfully. If not, an error message is printed and
 * the program exits.
 *
 * @param server The socket connected to the SMTP server.
 * @param expecting The 3-digit response code expected from the server.
 *
 * The function exits with an error if the connection drops, the
 * server response is too large, or if an unexpected response code
 * is received.
 */
void wait_on_response(SOCKET server, int expecting) {
  char response[MAXRESPONSE + 1];
  char *p = response;
  char *end = response + MAXRESPONSE;

  int code = 0;

  do {
    // WARN: server response is unlikely to come, due to the plethora of actions
    // taken by service providers to curb Spam plagues across the net.
    int bytes_received = recv(server, p, end - p, 0);
    if (bytes_received < 1) {
      fprintf(stderr, "Connection dropped.\n");
      exit(EXIT_FAILURE);
    }

    p += bytes_received;
    *p = 0;
    if (p == end) {
      fprintf(stderr, "Server response too large:\n");
      fprintf(stderr, "%s", response);
      exit(EXIT_FAILURE);
    }

    code = parse_response(response);
  } while (code == 0);

  if (code != expecting) {
    fprintf(stderr, "Error from server:\n");
    fprintf(stderr, "%s", response);
    exit(EXIT_FAILURE);
  }
  // S: Server
  printf("S: %s\n", response);
}

/**
 * @brief Connects to a remote host using a TCP socket.
 *
 * This function configures the address information for the specified
 * hostname and port, creates a TCP socket, and attempts to establish
 * a connection to the remote host. If successful, it returns the socket
 * descriptor. If any step fails, the function prints an error message
 * and exits the program.
 *
 * @param hostname The hostname or IP address of the remote host.
 * @param port The port number to connect to on the remote host.
 * @return The socket descriptor for the connection.
 *
 * @note The caller is responsible for closing the socket after use.
 */
SOCKET connect_to_host(const char *hostname, const char *port) {
  printf("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo *peer_address;
  if (getaddrinfo(hostname, port, &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Remote address is:\n");
  char address_buffer[100];
  char service_buffer[100];
  getnameinfo(peer_address->ai_addr, peer_address->ai_addrlen, address_buffer,
              sizeof(address_buffer), service_buffer, sizeof(service_buffer),
              NI_NUMERICHOST);
  printf("%s %s\n", address_buffer, service_buffer);

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
