// ch05-hostname-resolution-and-dns/dns_query/dns_query.c

#include "../../mylib/omniplat.h"
#include "../chap05.h"
#include "print_api.h"

/* WARNING: OS little-endian vs DNS Protocol big-endian
 * DNS Protocol specifies high-order byte is sent first. As an example:
 * When written to memory the 2-byte size decimal integer 999 convert to 0x03E7
 * and is subject to OS little-endianness rule specifying least significant
 * bytes occupy lowest memory addresses. As a result 999 is commit to memory as
 * "memory_cells" = {[0]= "E7", [1]= "03"}. And send in the same order [E7, 03]
 * accros the network by function like send() and recv().
 * However you should think of the DNS Protocol as a human asking for a number
 * he needs to write down. When you ask the year 2025 you do not expect to hear
 * 5, 2, 0 and 2 as little-endianness demands. DNS Protocol expects to be told
 * 2025 before it writes 2, 0, 5 and then 2. Therefore if you were to send 999
 * through DNS query you are required to send [03, E7]. And the same goes for
 * every single bytes received from a DNS message.
 * */

/**
 * @brief The main function is the entry point for this application.
 * @param argc The number of command line arguments.
 * @param argv The command line arguments.
 * @return EXIT_SUCCESS if successful, EXIT_FAILURE otherwise.
 *
 * @desc This program takes a hostname and a record type as command line
 * arguments, performs a DNS query to the Google public DNS server at 8.8.8.8,
 * and prints the DNS response message.
 *
 * Note that UDP is not always reliable. If our DNS query is lost in transit,
 * then dns_query hangs while waiting forever for a reply that never comes.
 * This could be fixed by using the function select() to time out and retry.
 */
int main(int argc, char *argv[]) {
  basename(&argv[0]);
  // Check if the user provided a hostname and record type
  if (argc < 3) {
    printf("Usage:\t\t%s hostname type\n", argv[0]);
    printf("Example:\t%s example.com aaaa\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

  // Make sure the hostname isn't too long
  if (strlen(argv[1]) > 255) {
    fprintf(stderr, "Hostname too long.\n");
    exit(EXIT_FAILURE);
  }

  // Try to interpret and read in the record type requested by the user
  unsigned char type;
  if (strcmp(argv[2], "a") == 0) {
    type = 1;
  } else if (strcmp(argv[2], "mx") == 0) {
    type = 15;
  } else if (strcmp(argv[2], "txt") == 0) {
    type = 16;
  } else if (strcmp(argv[2], "aaaa") == 0) {
    type = 28;
  } else if (strcmp(argv[2], "any") == 0) {
    // All cached record: unlikely to yield them all du to UDP unreliability
    type = 255;
  } else {
    fprintf(stderr,
            "Unknown type '%s'. Use: 'a', 'aaaa', 'mx', 'txt', or 'any'.\n",
            argv[2]);
    exit(EXIT_FAILURE);
  }

  // Initialize Winsock
#if defined(_WIN32)
  WSADATA WSAData;
  unsigned short wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WSAStartup(wVersionRequested, &WSAData);
  if (wsaerr) {
    fprintf(stderr, "Failed to initialize Winsock. The dll not found.\n");
    exit(EXIT_FAILURE);
  } else {
    printf("The dll found.\n");
    printf("The Status: %s\n", WSAData.szSystemStatus);
  }
#endif

  // Configure DNS address
  printf("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *peer_address;
  // INFO: Here getaddrinfo() is used to convert from a text IP address. In
  // spite of its synchronous nature that makes it a blocking function, since no
  // network communication is involved in this case getaddrinfo() recognizes the
  // name as resolved, thus it swiftly perfoms the local binary parsing to
  // convert it to the format required for network sockets.

  /* int gai_err = getaddrinfo("8.8.8.8", "53", &hints, &peer_address); */
  /* int gai_err = getaddrinfo("1.1.1.1", "53", &hints, &peer_address); */
  int gai_err = getaddrinfo("208.67.222.222", "53", &hints, &peer_address);
  if (gai_err) {
    fprintf(stderr, "getaddrinfo() failed. (%s)\n", gai_strerror(gai_err));
    exit(EXIT_FAILURE);
  }

  // Create socket
  printf("Creating socket...\n");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (BAD_SOCKET(socket_peer)) {
    REPORT_SOCKET_ERROR("socket() failed");
    exit(EXIT_FAILURE);
  }

  // Construct the data for the DNS message starting with the 12-byte header
  char query[1024] = {
      0xAB, 0xCD, /* ID */
      0x01,       /* Bitwise Flags: (1)qr, (4)opcode, (1)aa, (1)tc, (1)rd */
      0x00,       /* Reponse-type: RCODE */
      0x00, 0x01, /* Questions: QDCOUNT */
      0x00, 0x00, /* Answer RRs: ANCOUNT */
      0x00, 0x00, /* Authority RRs: NSCOUNT */
      0x00, 0x00, /* Additional RRs: ARCOUNT */
  };

  // Append user's desired hostname into the encoding of the DNS query message
  char *p = query + 12;
  char *h = argv[1];

  while (*h) {
    char *len = p;
    p++;              // Skip the length byte left for len to point to.
    if (h != argv[1]) // Then we are past the first label and we need to
      ++h;            // skip the dot reached in the preceding iteration
    while (*h && *h != '.')
      *p++ = *h++;
    *len = p - (len + 1);
  }
  *p++ = '\0';

  // Tail the encoding of the DNS query message with question type and class
  *p++ = 0x00;
  *p++ = type; /* QTYPE */
  *p++ = 0x00;
  *p++ = 0x01; /* QCLASS */
  // HACK: Without incrementing p in the previous statement the substraction
  // p - query would seem perfectly logical to give the difference from right
  // after query till the actual position of p (unincremented): in such a case
  // the first byte at the actual position of query is not counted. That is why
  // even if ultimately incrementing p seems wrong, because it places p where no
  // query information was commited, it is a free computing trick to enable p to
  // account for that first byte and accurately give the exact size of the
  // query.
  const int query_size = p - query;

  // Transmit the DNS query to the DNS server
  int bytes_sent = sendto(socket_peer, query, query_size, 0,
                          peer_address->ai_addr, peer_address->ai_addrlen);
  printf("Send %d bytes.\n", bytes_sent);

  // HACK: For debugging purposes display the query just sent to make sure there
  // was no mistake in query encoding
  print_dns_message(query, query_size);

  // Now that the query has been sent, we await a DNS response message using
  // recvfrom(). In a practical program, you may want to use select() here to
  // time out. It could also be wise to listen for additional messages in the
  // case that an invalid message is received first
  char read[1024];
  // INFO: See ch05-hostname-resolution-and-dns/NOTE_DNS.md for the way
  // recvfrom() is used in the specific contex of this program.
  // The last two arguments enable the function to provide you with the address
  // of the data sender. But since that sender was hard coded while populating
  // getaddrinfo() parameters, In the specific contex of this program
  // communication a strong assumption (in total disregard of malicious senders
  // or ill-formated packets) is made that we do know the origin of all data.
  // The zeros tell recvfrom(): "I don't care who is the sender, just give me
  // the data".
  int bytes_received = recvfrom(socket_peer, read, 1024, 0, 0, 0);
  printf("Received %d bytes.\n", bytes_received);
  print_dns_message(read, bytes_received);
  printf("\n");

  // Cleanup routines
  freeaddrinfo(peer_address);
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  return EXIT_SUCCESS;
}
