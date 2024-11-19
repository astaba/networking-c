/* dns_query.c */

#include "../mylib/omniplat.h"
#include <stdlib.h>

void print_dns_message(const char *message, int msg_length);
const unsigned char *print_name(const unsigned char *msg,
                                const unsigned char *p,
                                const unsigned char *end);

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
 * This could be fixed by using the function to time out and retry. select()
 */
int main(int argc, char *argv[]) {

  // Check if the user provided a hostname and record type
  if (argc < 3) {
    printf("Usage:\n\tdns_query hostname type\n");
    printf("Example:\n\tdns_query example.com aaaa\n");
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
    // All cached record: unlikely to yield them all du to UPD unreliability
    type = 255;
  } else {
    fprintf(stderr, "Unknown type '%s'. Use a, aaa, txt, mx, or any.", argv[2]);
    exit(EXIT_FAILURE);
  }

  // Initialize Winsock
#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return EXIT_SUCCESS;
  }
#endif

  // Configure DNS address
  printf("Configuring remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  struct addrinfo *peer_address;
  if (getaddrinfo("8.8.8.8", "53", &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_FAILURE;
  }

  // Create socket
  printf("Creating socket...\n");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    return 1;
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
    p++;
    if (h != argv[1])
      ++h;
    while (*h && *h != '.')
      *p++ = *h++;
    *len = p - len - 1;
  }
  *p++ = 0;

  // Tail the encoding of the DNS query message with question type and class
  *p++ = 0x00;
  *p++ = type; /* QTYPE */
  *p++ = 0x00;
  *p++ = 0x01; /* QCLASS */

  // Calculate the query size
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

/**
 * @brief print a dns message.
 * @param message: points to the start of the message
 * @param msg_length: is the length of the message
 *
 * @desc: this function is used to print an entire dns message to the screen.
 * dns messages share the same format for both the request and the response, so
 * our function is able to print either.
 * */
void print_dns_message(const char *message, int msg_length) {
  // First check that the message is long enough to be a valid dns message.
  // recall that the dns header is 12 bytes long. if a dns message is less than
  // 12 bytes, we can easily reject it as an invalid message. this also ensures
  // that we can read at least the header without worrying about reading past
  // the end of the received data.
  if (msg_length < 12) {
    fprintf(stderr, "message is too short to be valid.\n");
    exit(EXIT_FAILURE);
  }

  // Copy the message pointer into a new variable msg defined as an unsigned
  // char pointer, which makes certain calculations easier to work with.
  const unsigned char *msg = (const unsigned char *)message;

  // HACK: if you are curious about the raw dns message you can optionally print
  // it. bear in mind that since it prints out many lines it can be annoying.
  for (int i = 0; i < msg_length; ++i) {
    unsigned char r = msg[i];
    printf("%02d: %02x %03d '%c'\n", i, r, r, r);
  }
  printf("\n");

  // NOTE: In the subsequent comments bits are numerically qualified in the big
  // endian order.

  // Print the message id in a nice hexadecimal format
  // message ID -> 1st and 2nd bytes of the message
  printf("ID = %0x %0x\n", msg[0], msg[1]);

  // Next, we get the qr (Question/Response) bit from the message header. this
  // bit is the most significant bit of msg[2]. we use the bitmask 0x80 to see
  // whether it is set which it is a response otherwise it is a query.
  // QR -> 1st bit in the 3rd byte of the message
  const int qr = (msg[2] & 0x80) >> 7;
  printf("QR = %d %s\n", qr, qr ? "response" : "query");

  // The opcode indicates the type of query
  // opcode -> 4 bits after the 1st in the 3rd byte of the message
  const int opcode = (msg[2] & 0x78) >> 3;
  printf("OPCODE = %d ", opcode);
  // clang-format off
  switch (opcode) {
    case 0: printf("standard\n"); break;
    case 1: printf("reverse\n"); break;
    case 2: printf("status\n"); break; // Server status request
    default: printf("?\n"); break;     // 3 through 15 are reserved
  }
  // clang-format on
  // aa -> 6th bit in the 3rd byte of the message
  const int aa = (msg[2] & 0x04) >> 2;
  printf("AA = %d %s\n", aa, aa ? "authoritative" : "");
  // If tc is set the message truncated and sould be resent with TCP
  // tc -> 7th bit in the 3rd byte of the message
  const int tc = (msg[2] & 0x02) >> 1;
  printf("TC = %d %s\n", tc, tc ? "message truncated" : "");
  // Recursion is desired for the DNS server to contact additional servers
  // rd -> last bit in the 3rd byte of the message
  const int rd = (msg[2] & 0x01);
  printf("RD = %d %s\n", rd, rd ? "recursion desired" : "");

  // Finally, we can read in rcode for response-type messages. Since rcode can
  // have several different values, we use a switch statement to print them.
  if (qr) { // if it is a response
    // rcode -> 3 least significant bits in the 4th byte of the message
    const int rcode = msg[3] & 0x07;
    printf("RCODE = %d ", rcode);
    // clang-format off
    switch (rcode) {
      case 0: printf("success\n"); break;
      case 1: printf("format error\n"); break;
      case 2: printf("server failure\n"); break;
      case 3: printf("name error\n"); break;
      case 4: printf("not implemented\n"); break;
      case 5: printf("refused\n"); break;
      default: printf("?\n"); break;
    }
    // clang-format on
    if (rcode != 0)
      return;
  }

  // The next four fields in the header.
  // NOTE:
  // Implicit type promotion:
  // When you perform an operation like << on char or unsigned char values in c,
  // type promotion occurs. Specifically, char (or unsigned char) is promoted to
  // an int (or unsigned int if the original char was unsigned).
  // Effect of Promotion:
  // Because of this promotion, the shift occurs on the promoted type (int or
  // unsigned int), not directly on the char or unsigned char. This prevents the
  // value from being "zeroed out" if we shift by more bits than a char can
  // hold.
  // Example with msg[4] << 8:
  // msg[4] is of type unsigned char, which typically has a width of 8 bits
  // (1 byte). When msg[4] is promoted, it becomes an int (or unsigned int),
  // with a typical width of 32 bits (on most systems).
  const int qdcount = (msg[4] << 8) + msg[5];
  const int ancount = (msg[6] << 8) + msg[7];
  const int nscount = (msg[8] << 8) + msg[9];
  const int arcount = (msg[10] << 8) + msg[11];
  printf("QDCOUNT = %d\n", qdcount); // Question Count
  printf("ANCOUNT = %d\n", ancount); // Answer Record Count
  printf("NSCOUNT = %d\n", nscount); // Other Name Server (Authority) Count
  printf("ARCOUNT = %d\n", arcount); // Additional (info) Record Count

  // Before reading the rest of the message, we define two new variables: the p
  // variable is used to walk through the message. We set the end variable to
  // one past the end of the message. This is to help us detect whether we're
  // about to read past the end of the message – a situation we certainly wish
  // to avoid!
  const unsigned char *p = msg + 12;
  const unsigned char *end = msg + msg_length;

  // Read and print each question in the DNS message
  if (qdcount) {
    for (int i = 0; i < qdcount; ++i) {
      if (p >= end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      printf("Query %2d\n", i + 1);

      printf("%7s ", "name:");
      p = print_name(msg, p, end);
      printf("\n");
      if (p + 4 > end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      const int type = (p[0] << 8) + p[1];
      printf("%7s %d\n", "type:", type);
      p += 2;

      const int qclass = (p[0] << 8) + p[1];
      printf("%7s %d\n", "class:", qclass);
      p += 2;
    }
  }

  // Print the answer, authority and additional sections.
  if (ancount || nscount || arcount) {
    int i;
    for (i = 0; i < ancount + nscount + arcount; ++i) {
      if (p >= end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      printf("Answer %2d\n", i + 1);

      printf("%7s ", "name:");
      p = print_name(msg, p, end);
      printf("\n");

      if (p + 10 > end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      const int type = (p[0] << 8) + p[1];
      printf("%7s %d\n", "type:", type);
      p += 2;
      // qclass is for C++ friends who are not allowed "class" as varaible
      const int qclass = (p[0] << 8) + p[1];
      printf("%7s %d\n", "class:", qclass);
      p += 2;

      // Read TTL, how many seconds we are allowed to cached the answer, and
      const unsigned int ttl = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
      printf("%7s %u\n", "TTL:", ttl);
      p += 4;
      // Data length, how many bytes of additional data the answer includes
      const int rdlen = (p[0] << 8) + p[1];
      printf("%7s %d\n", "length:", rdlen);
      p += 2;
      // Before reading rdlen, check what we won't read past the answer
      if (p + rdlen > end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      // We can then try to interpret the answer data. Each record type stores
      // different data. We need to write code to display each type. For our
      // purposes, we limit this to the A, MX, AAAA, TXT, and CNAME records.
      if (rdlen == 4 && type == 1) { /* A: IPv4 address record */
        printf("%7s ", "Address");
        printf("%d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

      } else if (type == 15 && rdlen > 3) { /* MX: Mail exchange record */
        const int preference = (p[0] << 8) + p[1];
        printf("%7s %d\n", "pref:", preference);
        printf("%7s ", "MX:");
        print_name(msg, p + 2, end);
        printf("\n");

      } else if (rdlen == 16 && type == 28) { /* AAAA: IPv6 address record */
        printf("%7s ", "Address");
        int j;
        for (j = 0; j < rdlen; j += 2) {
          printf("%02x%02x", p[j], p[j + 1]);
          if (j + 2 < rdlen)
            printf(":");
        }
        printf("\n");

      } else if (type == 16) { /* TXT: Text record */
        printf("%7s '%.*s'\n", "TXT:", rdlen - 1, p + 1);

      } else if (type == 5) { /* CNAME: Canonical name */
        printf("%7s ", "CNAME:");
        print_name(msg, p, end);
        printf("\n");
      }
      p += rdlen;

    } // for (i = 0; i < ancount + nscount + arcount; ++i)
  } // if (ancount || nscount || arcount)

  if (p != end) {
    printf("There is some unread data left over.\n");
  }
  printf("\n");
}

/**
 * @brief: Print a DNS name.
 * @param msg: a pointer to the beginning of the message
 * @param p: a pointer to the name to print
 * @param end: a pointer to one past the end of the message.
 *
 * @desc: This function takes a pointer to the beginning of a DNS message and
 * prints out the DNS name that is stored in the message at the position p. The
 * end parameter is required so that we can check that we're not reading past
 * the end of the received message. The msg parameter is required for the same
 * reason, but also so that we can interpret name pointers.
 * */
const unsigned char *print_name(const unsigned char *msg,
                                const unsigned char *p,
                                const unsigned char *end) {
  // Check that a proper name is even possible. Because a name should consist of
  // at least a length and some text, we can return an error if p is already
  // within two characters of the end
  // We need to access at least one byte pointed to by p and another one bytes
  // before the end.
  if (p + 2 > end) {
    fprintf(stderr, "End of message.\n");
    exit(EXIT_FAILURE);
  }

  // We then check to see if p points to a name pointer. If it does, we
  // interpret the pointer and call print_name recursively to print the name
  // that is pointed to.
  if ((*p & 0xC0) == 0xC0) {
    const int k = ((*p & 0x3F) << 8) + p[1];
    p += 2;
    printf(" (pointer %d) ", k);
    print_name(msg, msg + k, end);
    return p;

  } else { // Length byte is not a name pointer: print it one lable at a time.
    const int len = *p++;
    // WARN: Previous checks may validate the start of a label, but the label
    // length is dynamic and could push `p + len + 1` beyond `end`, especially
    // if the length byte is corrupted or manipulated. This bounds check ensures
    // that data isn't read out of bounds, protecting against unexpected input,
    // corruption, or malicious attacks. Such checks are crucial for security
    // and stability in network data parsing.
    if (p + len + 1 > end) {
      fprintf(stderr, "End of message.\n");
      exit(EXIT_FAILURE);
    }

    printf("%.*s", len, p);
    p += len;

    if (*p) { // *p is either name length or name pointer
      printf(".");
      return print_name(msg, p, end);
    } else {
      return p + 1;
    }
  }
}
