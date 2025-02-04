/* 05_ip_lookup.c */

#include "../chap05/chap05.h"
#include <string.h>

#ifndef AI_ALL
#define AI_ALL 0x0100
#endif

/* A showcase program for the domain resolution capabilities of getaddrinfo(),
 * and reverse dns query capabilities of getnameinfo()
 * */
int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage:\n\tip_lookup hostname\n"
           "Example:\n\tip_lookup example.com\n");
    exit(EXIT_SUCCESS);
  }

#if defined(_WIN32)
  WSADATA wsaData;
  WORD wVersionRequested = MAKEWORD(2, 2);
  int wsaerr = WSAStartup(wVersionRequested, &wsaData);
  if (wsaerr) {
    fprintf(stderr, "The Winsock dll not found!\n");
    exit(EXIT_FAILURE);
  } else {
    printf("The Winsock dll found!\n");
    printf("The status: %s\n", wsaData.szSystemStatus);
  }
#endif

  printf("Resolving hostname: '%s'\n", argv[1]);
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_ALL;
  struct addrinfo *peer_address;
  if (getaddrinfo(argv[1], 0, &hints, &peer_address)) {
    fprintf(stderr, "getaddrinfo() failed. (%d)\n", GETSOCKETERRNO());
    exit(EXIT_FAILURE);
  }

  printf("Remote address is:\n");
  struct addrinfo *address = peer_address;
  do {
    struct sockaddr_storage addr_storage;
    memcpy(&addr_storage, address->ai_addr, address->ai_addrlen);

    char address_buffer[100];
    getnameinfo((struct sockaddr *)&addr_storage, address->ai_addrlen,
                address_buffer, sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    printf("\t%s\n", address_buffer);
  } while ((address = address->ai_next));

  freeaddrinfo(peer_address);

#if defined(_WIN32)
  WSACleanup();
#endif

  return EXIT_SUCCESS;
}
