// ch05-hostname-resolution-and-dns/dns_query/print_name.c

#include "../chap05.h"
#include "print_api.h"

/**
 * @brief: Prints a DNS name by recursively printing dot-separated labels
 * and returns the pointer to the byte immediately following the parsed name.
 * @param msg: a pointer to the beginning of the entire DNS message (needed for
 * resolving name pointers).
 * @param p: a pointer to the current position in the message, pointing to the
 * start of a name component (label length or pointer).
 * @param endafter: a pointer to one past the end of the message, used for
 * bounds checking.
 *
 * @description: This function has two distinct but equally important purposes:
 * (1) update the p pointer (used by the calling stack frame to track reading
 * advance in the message) on behalf of print_dns_msg and propagate it back.
 * (2) recursively print dot-separated labels to complete printing a name.
 * Example names: "www.yahoo.com", "me-ycpi-cf-www.g06.yahoodns.net".
 * Thus to print the preceding names the function will respectively incur in 3
 * and 4 recursive calls.
 *
 * BLOCK: if (p + 2 > endafter) {}.
 * A name label must start with 2 bytes in either of two ways:
 * (1) The most significant double bits are not set with the side effect that 6
 * bits are left to account for the char length of the current label therefore
 * bounded within 63 chars; then the second byte would hold the first character
 * of the label.
 * (2) The most significant double bits are set making the remaining 14 bits
 * container of an integer offset from the beginning of the msg as back
 * reference to an afore-mentioned name: this is the compression pointer.
 *
 * BLOCK: if ((*p & 0xC0) == 0xC0) {.
 * If print_dns_msg() sends a compression pointer, from its stand point it is
 * sending a back reference already traversed by p. As a result print_dns_msg()
 * does not need print_name() to advance p but is only interested in the
 * printing side effect of print_name(). As a result: we advance p by only the 2
 * bytes of the compression pointer; and curtail p update to the first stack
 * frame of print_name() to comply with the usage of p in the parent frame
 * reading the DNS message. That is why this recursive call is dangling with its
 * returned value ignored.
 *
 * BLOCK: } else {}.
 * In this case the calling print_dns_msg() sends p as length byte. As a result
 * print_name() must fulfill both function purposes: primarily, print the label
 * in all its length with a dot separator only if another label follows up and
 * recursively print it to complete the name; secondly, from print_dns_msg()
 * stand point p refers to the current name ahead of p and not behind in the DNS
 * message, therefore compelling print_name() to recursively propagate every p
 * update up back the stack frame till print_dns_msg(). That is why all
 * print_name() value is recursively returned.
 *
 * BLOCK: if (len == 0 && *p != 0) {}.
 * BLOCK: if (p + len + 1 > endafter) {}.
 * In case the length byte is neither corrupted nor manipulated perform next
 * bound check to ensure there are enough bytes remaining in the message to
 * read:
 * -  the purported label length;
 * -  and at least one byte for either the length of the next label or the
 * message null terminator;
 * protecting against unexpected input, corruption, or malicious attacks.
 * Such checks are crucial for security and stability in network data parsing.
 *
 * BLOCK: if (*p) {.
 * After printing the label and advancing p by len, *p means we have another
 * label. Print the dot and return the recursion to propagate p update.
 *
 * BLOCK: } else {.
 * We reached the whole name end at the null terminator. Return such as to
 * report back the position of p passed that '\0'.
 * */
const unsigned char *print_name(const unsigned char *msg,
                                const unsigned char *p,
                                const unsigned char *endafter) {
  if (p + 2 > endafter) {
    fprintf(stderr, "End of message (initial check)");
    exit(EXIT_FAILURE);
  }

  if ((*p & 0xC0) == 0xC0) {
    const ptrdiff_t k = ((*p & 0x3F) << 8) + p[1];
    p += 2;
    printf(" (pointer: %td) ", k);
    print_name(msg, msg + k, endafter);
    return p;

  } else {
    const int len = *p++;
    if (len == 0 && *p != 0) {
      fprintf(stderr, "End of message (corrupted length)");
      exit(EXIT_FAILURE);
    }
    if (p + len + 1 > endafter) {
      fprintf(stderr, "End of message (out-of-bound length)");
      exit(EXIT_FAILURE);
    }

    printf("%.*s", len, (char *)p);
    p += len;

    if (*p) {
      printf(".");
      return print_name(msg, p, endafter);
    } else {
      return p + 1;
    }
  }
}
