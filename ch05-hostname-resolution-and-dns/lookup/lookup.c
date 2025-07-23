// ch05-hostname-resolution-and-dns/lookup/lookup.c
/* @file lookup.c
 * @brief This program takes a name or IP address for its only argument. It then
 * uses getaddrinfo() to resolve that name or that IP address into an address
 * structure, and the program prints that IP address using getnameinfo() for the
 * text conversion. If multiple addresses are associated with a name, it prints
 * each of them. It also indicates any errors.
 * */

#include "../../mylib/omniplat.h"
#include "../chap05.h"

int main(int argc, char *argv[]) {
  basename(&argv[0]);
  if (argc < 2) {
    printf("Usage:\t\t%s <hostname | IPaddress>\n", argv[0]);
    printf("Example:\t%s example.com\n", argv[0]);
    printf("Example:\t%s 196.22.68.1\n", argv[0]);
    exit(EXIT_SUCCESS);
  }

#if defined(_WIN32)
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsa_error = WSAStartup(wVersionRequested, &WSAData);
  if (wsa_error) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  printf("Resolving hostname: '%s'\n", argv[1]);
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_ALL;

  struct addrinfo *peer_address;
  int gai_err = getaddrinfo(argv[1], 0, &hints, &peer_address);
  if (gai_err) {
    fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(gai_err));
    exit(EXIT_FAILURE);
  }

  printf("Remote address is:\n");
  struct addrinfo *address = peer_address;
  do {
    char address_buffer[100];
    int gni_err =
        getnameinfo(address->ai_addr, address->ai_addrlen, address_buffer,
                    sizeof(address_buffer), 0, 0, NI_NUMERICHOST);
    if (gni_err) {
      fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(gni_err));
      exit(EXIT_FAILURE);
    }
    printf("\t%s\n", address_buffer);
  } while ((address = address->ai_next));

  freeaddrinfo(peer_address);

#if defined(_WIN32)
  WSACleanup();
#endif

  return EXIT_SUCCESS;
}
