/* @file lookup.c
 * @brief This program takes a name or IP address for its only argument. It then
 * uses getaddrinfo() to resolve that name or that IP address into an address
 * structure, and the program prints that IP address using getnameinfo() for the
 * text conversion. If multiple addresses are associated with a name, it prints
 * each of them. It also indicates any errors.
 * */

#include "chap05.h"

#ifndef AI_ALL
#define AI_ALL 0x0100
#endif

int main(int argc, char *argv[]) {

  if (argc < 2) {
    printf("Usage:\n\tlookup hostname\n");
    printf("Example:\n\tlookup example.com\n");
    exit(EXIT_SUCCESS);
  }

#if defined(_WIN32)
  WSADATA d;
  if (WSAStartup(MAKEWORD(2, 2), &d)) {
    fprintf(stderr, "Failed to initialize.\n");
    return EXIT_FAILURE;
  }
#endif

  // Convert hostname into a struct addrinfo
  printf("Resolving hostname '%s'\n", argv[1]);
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_ALL;
  struct addrinfo *peer_address;
  if (getaddrinfo(argv[1], 0, &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    return EXIT_SUCCESS;
  }

  // Convert address(es) to numeric text
  printf("Remote addres is:\n");
  struct addrinfo *address = peer_address;
  do {
    char address_buffer[100];
    getnameinfo(address->ai_addr, address->ai_addrlen, address_buffer,
                sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("\t%s\n", address_buffer);
  } while ((address = address->ai_next));

  freeaddrinfo(peer_address);

#if defined(_WIN32)
  WSACleanup();
#endif

  return EXIT_SUCCESS;
}
