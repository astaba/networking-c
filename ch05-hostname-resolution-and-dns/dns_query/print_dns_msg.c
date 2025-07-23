// ch05-hostname-resolution-and-dns/dns_query/print_dns_msg.c

#include "../chap05.h"
#include "print_api.h"

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
    fprintf(stderr, "Message is too short to be valid.\n");
    exit(EXIT_FAILURE);
  }

  // Copy the message pointer into a new variable msg defined as an unsigned
  // char pointer, which makes certain calculations easier to work with.
  const unsigned char *msg = (const unsigned char *)message;

  // HACK: if you are curious about the raw dns message you can optionally print
  // it. bear in mind that since it prints out many lines it can be annoying.
  int i;
  for (i = 0; i < msg_length; ++i) {
    unsigned char r = msg[i];
    printf("%02d: %02x %03d '%c'\n", i, r, r, r);
  }
  printf("\n");

  // NOTE: In the subsequent comments and code statements bits and bytes are
  // numerically qualified accroding to DNS Protocol big-endiannes not the OS
  // little-endianness.

  // Print the message id in a nice hexadecimal format
  // message ID -> 1st and 2nd bytes of the message
  printf("ID = %#0x%0x\n", msg[0], msg[1]);

  // Next, we get the qr bit from the message header. this bit is the most
  // significant bit of msg[2]. we use the bitmask 0x80 to test whether it is
  // set (a response) or not (a query).
  // QR -> 1st bit in the 3rd byte of the message
  const int qr = (msg[2] & 0x80) >> 7;
  printf("QR = %d %s\n", qr, qr ? "response" : "query");

  // The opcode, aa, tc, and rd fields are read in much the same way as qr.
  // OPCODE -> 4 bits after the 1st in the 3rd byte of the message
  const int opcode = (msg[2] & 0x78) >> 3;
  printf("OPCODE = %d ", opcode);
  // clang-format off
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

  // Finally, we can read in rcode for response-type messages. Since rcode can
  // have several different values, we use a switch statement to print them.
  if (qr) { // if it is a response
    // ra -> first most significant bit in the 4th byte of the message
    const int ra = (msg[3] & 0x80) >> 7;
    printf("RA = %d %s\n", ra, ra ? "recursion available" : "");

    // rcode -> 3 least significant bits in the 4th byte of the message
    const int rcode = msg[3] & 0x0F;
    printf("RCODE = %d ", rcode);
    // clang-format off
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
  printf("NSCOUNT = %d\n", nscount); // Name Server (Authority) Count
  printf("ARCOUNT = %d\n", arcount); // Additional Record Count

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

  // Print the answer, authority and additional sections.
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
      // qclass is for C++ friends who are not allowed "class" as varaible
      const int qclass = (p[0] << 8) + p[1];
      printf("\t%7s %d\n", "class:", qclass);
      p += 2;

      // Read TTL, how many seconds we are allowed to cached the answer, and
      const unsigned int ttl = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
      printf("\t%7s %u\n", "TTL:", ttl);
      p += 4;
      // Data length, how many bytes of additional data the answer includes
      const int rdlen = (p[0] << 8) + p[1];
      printf("\t%7s %d\n", "length:", rdlen);
      p += 2;
      // Before reading rdlen, check what we won't read past the answer
      if (p + rdlen > end) {
        fprintf(stderr, "End of message.\n");
        exit(EXIT_FAILURE);
      }

      // We can then try to interpret the answer data. Each record type stores
      // different data. We need to write code to display each type. For our
      // purposes, we limit this to the A, MX, AAAA, TXT, and CNAME records.
      if (rdlen == 4 && type == 1) { /* A Record */
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
        // WARN: The following code snippet to parse TXT record does not account
        // for a TXT made up of more that one {[length_byte][string]}
        // printf("\t%7s '%.*s'\n", "TXT:", rdlen - 1, p + 1);
        // INFO: According to RFC 1035, Section 3.3.14 (TXT RDATA format) a
        // single TXT record's RDATA can legitimately look like this:
        // [length_byte1][string1][length_byte2][string2][length_byte3][string3]...

        printf("\t%7s ", "TXT:");
        const unsigned char *sub_string = p; // Local scope reading pointer
        // Total bytes left to parse from the reading pointer within RDATA
        int bytes_remaining = rdlen;

        while (bytes_remaining > 0) {
          if (sub_string >= end) { // Basic bounds check for current part
            fprintf(stderr, "Error: TXT record data unexpectedly ends.\n");
            exit(EXIT_FAILURE);
          }
          const int string_len = sub_string[0]; // current string length byte
          sub_string++;      // Skip the length byte to the actual string data
          bytes_remaining--; // Decrement bytes count accordingly

          if (string_len > bytes_remaining) { // bound check
            fprintf(stderr, "Error: TXT record string length exceeds remaining "
                            "RDLENGTH.\n");
            exit(EXIT_FAILURE);
          }

          printf("'%.*s'", string_len, sub_string); // Print the string

          sub_string += string_len;      // Advance reading pointer accordingly
          bytes_remaining -= string_len; // Decrement bytes count accordingly
          if (bytes_remaining > 0) {
            // Then TXT record has another [length_byte][string]
            printf(" "); // Add a separator between TXT record strings
          }
        }
        printf("\n"); // Newline after all strings are printed
        // End of TXT Record parsing

      } else if (type == 5) { /* CNAME Record */
        printf("\t%7s ", "CNAME:");
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
