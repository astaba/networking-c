/* networking/ch05-hostname-resolution-and-dns/test/test_prt_pointer.c */

#include "../chap05.h"
#include "../dns_query/print_api.h"

int main() {
  // NOTE: Sole purpose of this variable is to contextualize the DNS response
  // --- DNS Query Message Example ---
  // This query asks for the A record (IPv4 address) of "www.google.com"
  // Header:
  //   ID:      0x1234 (arbitrary)
  //   Flags:   0x0100 (QR=0=Query, OpCode=0=Standard Query, RD=1=Recursion
  //   Desired)
  //   QDCOUNT: 1, ANCOUNT, NSCOUNT, ARCOUNT: 0
  // Question:
  //   QNAME:   "www.google.com" (3www6google3com0)
  //   QTYPE:   A (0x0001)
  //   QCLASS:  IN (0x0001)
  unsigned char dns_query_message[] = {
      // HEADER (12 bytes)
      0x12, 0x34, // ID (arbitrary)
      0x01,
      0x00, // Flags (QR=0, OpCode=0, AA=0, TC=0, RD=1, RA=0, Z=0, RCODE=0)
      0x00, 0x01, // QDCOUNT (1 Question)
      0x00, 0x00, // ANCOUNT (0 Answers)
      0x00, 0x00, // NSCOUNT (0 Authority Records)
      0x00, 0x00, // ARCOUNT (0 Additional Records)

      // QUESTION SECTION (QNAME + QTYPE + QCLASS)
      // QNAME: www.google.com
      3, 'w', 'w', 'w', 6, 'g', 'o', 'o', 'g', 'l', 'e', 3, 'c', 'o', 'm',
      0, // Null terminator for QNAME

      // QTYPE: A (Host Address)
      0x00, 0x01,

      // QCLASS: IN (Internet)
      0x00, 0x01};

  // INFO: Example: "foo.bar.com" compressed to "foo.(pointer to .bar.com)"
  // msg: [Header][...]
  //       Offset 0x0C: 3 foo (name starts here)
  //       Offset 0x10: 3 bar
  //       Offset 0x14: 3 com
  //       Offset 0x18: 0 (end of bar.com)
  //       Offset 0x19: 3 fuz
  //       Offset 0x1D: C0 10 (pointer to 0x10, which is "bar.com")
  //       Offset 0x1F: 0 (end of fuz.bar.com)

  // NOTE: --- Simulate full DNS Response Message with a compressed name ---
  // This is a response to the query above, providing an A record for
  // "www.google.com" and demonstrating a name compression pointer.
  // Header:
  //   ID:       0x1234 (matches query)
  //   Flags:    0x8180 (QR=1=Response, OpCode=0=Standard Query, AA=0, TC=0,
  //             RD=1, RA=1, Z=0, RCODE=0=No Error)
  //   QDCOUNT:  1
  //   ANCOUNT:  1
  //   NSCOUNT,  ARCOUNT: 0
  // Question:   (Echoed from query)
  //   QNAME:    "www.google.com"
  //   QTYPE:    A
  //   QCLASS:   IN
  // Answer:
  //   NAME:     Pointer to QNAME (0xC00C, pointing to offset 0x0C, where
  //             "www.google.com" starts)
  //   TYPE:     A (Host Address)
  //   CLASS:    IN (Internet)
  //   TTL:      300 seconds (0x0000012C)
  //   RDLENGTH: 4 bytes (for IPv4 address)
  //   RDATA:    172.217.160.106 (an example Google IP)
  unsigned char dns_response_message[] = {
      // HEADER (12 bytes)
      0x12, 0x34, // ID (matches query)
      0x81,
      0x80, // Flags (QR=1, OpCode=0, AA=0, TC=0, RD=1, RA=1, Z=0, RCODE=0)
      0x00, 0x01, // QDCOUNT (1 Question)
      0x00, 0x01, // ANCOUNT (1 Answer)
      0x00, 0x00, // NSCOUNT (0 Authority Records)
      0x00, 0x00, // ARCOUNT (0 Additional Records)

      // QUESTION SECTION (echoed from query, must be identical to query's
      // question)
      // QNAME: www.google.com
      3, 'w', 'w', 'w',                // 0x0C: www
      6, 'g', 'o', 'o', 'g', 'l', 'e', // 0x10: google
      3, 'c', 'o', 'm',                // 0x17: com
      0, // 0x1B: Null terminator for www.google.com
      // QTYPE: A (Host Address)
      0x00, 0x01,
      // QCLASS: IN (Internet)
      0x00, 0x01,

      // ANSWER SECTION (Starts after QCLASS of Question, which ends at offset
      // 0x0B (header) + 20 bytes (Q section) = 0x1F. So Answer starts at 0x20)

      /* From NAME to RDATA we have a whole "Resource Records" of either:
       * Answer, Authority or Additional sections repeatable as many times as
       * header.ANCOUNT + header.NSCOUNT + header.ARCOUNT
       * */

      // NAME: Pointer to the QNAME (offset 0x0C in the message)
      // its size in bytes is variable
      0xC0, 0x10, // C0 indicates pointer, 0C is the offset from msg_start
                  // (where www.google.com starts)
      // TYPE: A (Host Address) 2-byte
      0x00, 0x01,
      // CLASS: IN (Internet) 2-byte
      0x00, 0x01,
      // TTL: 300 seconds (0x0000012C) 4-byte
      0x00, 0x00, 0x01, 0x2C,
      // RDLENGTH: 2-byte
      0x00, 0x04,
      // RDATA: 172.217.160.106, as many bytes as specidied in RDLENGTH
      172, 217, 160, 106};

  printf("DNS Query Message size: %zu bytes\n", sizeof(dns_query_message));
  printf("DNS Response Message size: %zu bytes\n",
         sizeof(dns_response_message));
  puts("");

  const unsigned char *msg_start = dns_response_message;
  const unsigned char *buffer_end =
      dns_response_message + sizeof(dns_response_message);

  // Test Case 1: Parsing "www.google.com" (no pointers initially)
  const unsigned char *name1_start = msg_start + 12; // Starts after header
  printf("--- Parsing Name 1 (www.google.com) ---\n");
  const unsigned char *name1_end_ptr =
      print_name(msg_start, name1_start, buffer_end);
  printf("\nEnd of Name 1 returned pointer offset: %td\n\n",
         name1_end_ptr - msg_start);

  // Test Case 2: Parsing a name that starts with a pointer (e.g., the answer's
  // name)
  const unsigned char *name2_start = msg_start + 0x20; // Points to 0xC0 10
  printf("--- Parsing Name 2 (pointer to google.com) ---\n");
  const unsigned char *name2_end_ptr =
      print_name(msg_start, name2_start, buffer_end);
  printf("\nEnd of Name 2 returned pointer offset: %td\n\n",
         name2_end_ptr - msg_start);

  return 0;
}
