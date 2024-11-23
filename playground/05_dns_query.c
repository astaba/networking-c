/* 05_dns_query.c */

#include "../chap05/chap05.h"

const unsigned char *print_name(const unsigned char *msg,
                                const unsigned char *p,
                                const unsigned char *endafter);
void print_dns_message(const char *message, int msg_length);

int main(int argc, char *argv[]) {

  if (argc < 3) {
    printf("Usage:\n\tdns_query hostname record_type\n");
    printf("Example:\n\tdns_query example.com aaaa\n");
    exit(EXIT_SUCCESS);
  }

  if (strlen(argv[1]) > 255) {
    fprintf(stderr, "Hostname too long.\n");
    exit(EXIT_FAILURE);
  }

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
    type = 255;
  } else {
    fprintf(stderr,
            "Unknown type '%s'. Use: 'a', 'aaaa', 'mx', 'txt', or 'any'.\n",
            argv[2]);
    exit(EXIT_FAILURE);
  }

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

  printf("Configure remote address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_DGRAM;
  struct addrinfo *peer_address;
  if (getaddrinfo("8.8.8.8", "53", &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Create socket...\n");
  SOCKET socket_peer;
  socket_peer = socket(peer_address->ai_family, peer_address->ai_socktype,
                       peer_address->ai_protocol);
  if (!ISVALIDSOCKET(socket_peer)) {
    fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  // Construct the data for the DNS message
  char query[1024] = {
      0xAB, 0xCD, /* ID */
      0x01,       /* Bitwise Flags: (1)qr, (4)opcode, (1)aa, (1)tc, (1)rd */
      0x00,       /* Reponse-type: RCODE */
      0x00, 0x01, /* Questions: QDCOUNT */
      0x00, 0x00, /* Answer RRs: ANCOUNT */
      0x00, 0x00, /* Authority RRs: NSCOUNT */
      0x00, 0x00, /* Additional RRs: ARCOUNT */
  };

  // Encode user's desired hostname into the query
  char *p = query + 12;
  char *h = argv[1];

  while (*h) {
    char *len = p;
    p++;
    if (h != argv[1]) // Then we are past the first label and we need to
      ++h;            // skip the point reached in the preceding iteration
    while (*h && *h != '.')
      *p++ = *h++;
    *len = p - (len + 1);
  }
  *p++ = '\0';

  // Add question type and class to the query
  *p++ = 0x00;
  *p++ = type; /* QTYPE */
  *p++ = 0x00;
  *p++ = 0x01; /* QCLASS */

  // Calculate the query size
  const int query_size = p - query;

  // Send the query
  int bytes_sent = sendto(socket_peer, query, query_size, 0,
                          peer_address->ai_addr, peer_address->ai_addrlen);
  printf("Sent %d bytes.\n", bytes_sent);
  // Just for debugging we can display the query sent
  print_dns_message(query, query_size);

  char read[1024];
  int bytes_received = recvfrom(socket_peer, read, 1024, 0, 0, 0);
  printf("Received %d bytes.\n", bytes_received);

  print_dns_message(read, bytes_received);
  printf("\n");

  freeaddrinfo(peer_address);
  CLOSESOCKET(socket_peer);

#if defined(_WIN32)
  WSACleanup();
#endif

  return EXIT_SUCCESS;
}

/**
 * @brief Print an entire DNS message to the screen.
 *
 * @param message A pointer to the message beginning.
 * @param msg_length Indicates the message's length
 */
void print_dns_message(const char *message, int msg_length) {

  if (msg_length < 12) {
    fprintf(stderr, "Message is too short to be valid.\n");
    exit(EXIT_FAILURE);
  }

  const unsigned char *msg = (const unsigned char *)message;

  // Optionally print the raw DNS message one bytes at a time.
  /* int i;
  for (i = 0; i < msg_length; i++) {
    unsigned char r = msg[i];
    printf("%02d:   %02X  %03d  '%c'\n", i, r, r, r);
  }
  printf("\n"); */

  // 1st and  2nd bytes: message ID
  printf("ID = %0X %0X\n", msg[0], msg[1]);

  // 3rd byte, 1st most significant bit : QR query or response
  const int qr = (msg[2] & 0x80) >> 7;
  printf("QR = %d %s\n", qr, qr ? "response" : "query");
  // from 2nd on, 4 most significant bit: OPCODE
  const int opcode = (msg[2] & 0x78) >> 3;
  printf("OPCODE = %d ", opcode);
  switch (opcode) {
    // clang-format off
    case 0:  printf("standard\n"); break;
    case 1:  printf("reverse\n");  break;
    case 2:  printf("status\n");   break;
    default: printf("?\n");
    // clang-format on
  }
  // 3rd least significant bit: AA authoritative
  const int aa = (msg[2] & 0x04) >> 2;
  printf("AA = %d %s\n", aa, aa ? "authoritative" : "");
  // 2nd least significant bit: TC message truncated
  const int tc = (msg[2] & 0x02) > 1;
  printf("TC = %d %s\n", tc, tc ? "message truncated" : "");
  // Least significant bit: RD recursion desired.
  const int rd = (msg[2] & 0x01);
  printf("RD = %d %s\n", rd, rd ? "recursion desired" : "");

  if (qr) {
    // 4th byte 3 least significant bit.
    const int rcode = msg[3] & 0x07;
    printf("RCODE = %d ", rcode);
    switch (rcode) {
      // clang-format off
      case 0:  printf("success\n");         break;
      case 1:  printf("format error\n");    break;
      case 2:  printf("server failure\n");  break;
      case 3:  printf("name error\n");      break;
      case 4:  printf("not implemented\n"); break;
      case 5:  printf("refused\n");         break;
      default: printf("?\n");
      // clang-format on
    }
    if (rcode != 0)
      return;
  }

  const int qdcount = (msg[4] << 8) + msg[5];
  const int ancount = (msg[6] << 8) + msg[7];
  const int nscount = (msg[8] << 8) + msg[9];
  const int arcount = (msg[10] << 8) + msg[11];

  printf("QDCOUNT = %d\n", qdcount); // Question Count
  printf("ANCOUNT = %d\n", ancount); // Answer Record Count
  printf("NSCOUNT = %d\n", nscount); // Name Server (Authority) Count
  printf("ARCOUNT = %d\n", arcount); // Additional Record Count

  // NOTE: That concludes reading the 12 bytes of the DNS header.

  const unsigned char *p = msg + 12;
  const unsigned char *end = msg + msg_length;

  if (qdcount) {
    int i;
    for (i = 0; i < qdcount; i++) {
      if (p >= end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      printf("Query %2d\n", i + 1);

      printf("\t%7s ", "name:");
      p = print_name(msg, p, end);
      printf("\n");

      if (p + 4 > end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      const int type = (p[0] << 8) + p[1];
      printf("\t%7s %d\n", "type:", type);
      p += 2;

      const int qclass = (p[0] << 8) + p[1];
      printf("\t%7s %d\n", "class:", qclass);
      p += 2;
    }
  }

  if (ancount || nscount || arcount) {

    int i;
    for (i = 0; i < (ancount + nscount + arcount); i++) {
      if (p >= end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      printf("Answer %2d\n", i + 1);

      printf("\t%7s ", "name:");
      p = print_name(msg, p, end);
      printf("\n");

      if (p + 10 > end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      const int type = (p[0] << 8) + p[1];
      printf("\t%7s %d\n", "type:", type);
      p += 2;

      const int qclass = (p[0] << 8) + p[1];
      printf("\t%7s %d\n", "class:", qclass);
      p += 2;

      const unsigned int ttl = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
      printf("\t%7s %u\n", "TTL:", ttl);
      p += 4;

      const int rdlen = (p[0] << 8) + p[1];
      printf("\t%7s %d\n", "length:", rdlen);
      p += 2;

      if (p + rdlen > end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      if (rdlen == 4 && type == 1) { /* A record */
        printf("\t%7s ", "Address");
        printf("%d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

      } else if (type == 15 && rdlen > 3) { /* MX Record */
        const int preference = (p[0] << 8) + p[1];
        printf("\t%7s %d\n", "Pref:", preference);
        printf("\t%7s ", "MX:");
        print_name(msg, p + 2, end);
        printf("\n");

      } else if (rdlen == 16 && type == 28) { /* AAAA Record */
        printf("\t%7s ", "Address");
        int j;
        for (j = 0; j < rdlen; j += 2) {
          printf("%02x%02x", p[j], p[j + 1]);
          if (j + 2 < rdlen)
            printf(":");
        }
        printf("\n");

      } else if (type == 16) { /* TXT Record */
        printf("\t%7s '%.*s'\n", "TXT:", rdlen - 1, p + 1);

      } else if (type == 5) { /* CNAME Record */
        printf("\t%7s ", "CNAME:");
        print_name(msg, p, end);
        printf("\n");
      }

      p += rdlen;
    }
  }

  if (p != end)
    printf("There is some unread data left over.\n");
  printf("\n");
}

/**
 * @brief A function to print name labels from a DNS message.
 *
 * @param msg A pointer to the message beginning.
 * @param p A pointer to the name to print.
 * @param endafter A pointer to one past the end of the message.
 */
const unsigned char *print_name(const unsigned char *msg,
                                const unsigned char *p,
                                const unsigned char *endafter) {

  if (p + 2 > endafter) {
    fprintf(stderr, "End of message.\n");
    exit(EXIT_FAILURE);
  }

  if ((*p & 0xC0) == 0xC0) {
    const int k = ((*p & 0x3F) << 8) + p[1];
    p += 2;
    printf(" (pointer: %d) ", k);
    print_name(msg, msg + k, endafter);
    return p;

  } else {
    const int len = *p++;
    // NOTE: Check that there are enough bytes remaining in the message to read:
    // -  the purported label length;
    // -  and at least one byte for either the length of the next label or the
    // message null terminator.
    if (p + len + 1 > endafter) {
      fprintf(stderr, "End of message.\n");
      exit(EXIT_FAILURE);
    }

    printf("%.*s", len, p);
    p += len;
    if (*p) {
      printf(".");
      return print_name(msg, p, endafter);

    } else {
      return p + 1;
    }
  }
}
