/* networking\chap01\win_list.c */

// List network adapters on Windows

#include <minwindef.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x600
#endif /* ifndef _WIN32_WINNT                                                  \
#define _WIN32_WINNT 0x600*/

#include <winsock2.h> // Winsock API
#include <ws2tcpip.h>
/* ==  prevent lsp format reordering which triggers nasty errors  =========== */
#include <iphlpapi.h> // IP Helper API
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

int main(int argc, char *argv[]) {
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsa_error = WSAStartup(wVersionRequested, &WSAData);
  if (wsa_error) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }

  DWORD asize = 15000;
  PIP_ADAPTER_ADDRESSES adapters = NULL;
  ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
  do {
    adapters = (PIP_ADAPTER_ADDRESSES)malloc(asize);

    if (!adapters) {
      fprintf(stderr, "Could not allocate %ld bytes for adapters.\n", asize);
      WSACleanup();
      return -1;
    }

    int result = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, adapters, &asize);

    if (result == ERROR_BUFFER_OVERFLOW) {
      // Then asize was updated within GetAdaptersAddresses with the size needed
      // to retry on the next iteration with a bigger buffer
      printf("GetAdaptersAddresses wants %ld bytes.\n", asize);
      free(adapters);
      adapters = NULL;
    } else if (result == ERROR_SUCCESS) {
      break;
    } else {
      printf("Error from GetAdaptersAddresses: %d\n", result);
      free(adapters);
      WSACleanup();
      return -1;
    }
  } while (!adapters);

  PIP_ADAPTER_ADDRESSES adapter = adapters;
  while (adapter) {
    printf("\nAdapter name: %S\n", adapter->FriendlyName);

    PIP_ADAPTER_UNICAST_ADDRESS address = adapter->FirstUnicastAddress;
    while (address) {
      printf("\t%s", address->Address.lpSockaddr->sa_family == AF_INET
                         ? "IPv4"
                         : "IPv6");
      char ap[100];
      getnameinfo(address->Address.lpSockaddr, address->Address.iSockaddrLength,
                  ap, sizeof(ap), 0, 0, NI_NUMERICHOST);
      printf("\t%s\n", ap);

      address = address->Next;
    }
    adapter = adapter->Next;
  }

  free(adapters);
  WSACleanup();
  return EXIT_SUCCESS;
}
