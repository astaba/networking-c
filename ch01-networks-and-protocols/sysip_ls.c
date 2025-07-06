/* list_ip.c */

#ifdef _WIN32
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
#else
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#endif

int main(int argc, char *argv[]) {
#ifdef _WIN32
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

  while (1) {
    adapters = (PIP_ADAPTER_ADDRESSES)malloc(asize);
    if (!adapters) {
      fprintf(stderr, "Could not allocate %ld bytes for adapters.\n", asize);
      WSACleanup();
      exit(-1);
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
      exit(-1);
    }
  }

  for (PIP_ADAPTER_ADDRESSES adapter = adapters; adapter;
       adapter = adapter->Next) {
    wprintf(L"\nAdapter name: %s\n", adapter->FriendlyName);
    for (PIP_ADAPTER_UNICAST_ADDRESS addr = adapter->FirstUnicastAddress; addr;
         addr = addr->Next) {
      printf("  %s",
             addr->Address.lpSockaddr->sa_family == AF_INET ? "IPv4" : "IPv6");
      char ipstr[100];
      getnameinfo(addr->Address.lpSockaddr, addr->Address.iSockaddrLength,
                  ipstr, sizeof(ipstr), NULL, 0, NI_NUMERICHOST);
      printf("  %s\n", ipstr);
    }
  }

  free(adapters);
  WSACleanup();
#else
  // Unix code
  struct ifaddrs *addresses = NULL;
  if (getifaddrs(&addresses) == -1) {
    perror("getifaddrs call failed");
    return EXIT_FAILURE;
  }

  for (struct ifaddrs *addr = addresses; addr; addr = addr->ifa_next) {
    if (!addr->ifa_addr)
      continue;
    int family = addr->ifa_addr->sa_family;
    if (family == AF_INET || family == AF_INET6) {
      printf("%s\t", addr->ifa_name);
      printf("%s\t", family == AF_INET ? "IPv4" : "IPv6");

      char ipaddr_str[100];
      socklen_t addr_len = family == AF_INET ? sizeof(struct sockaddr_in)
                                             : sizeof(struct sockaddr_in6);
      getnameinfo(addr->ifa_addr, addr_len, ipaddr_str, sizeof(ipaddr_str),
                  NULL, 0, NI_NUMERICHOST);
      printf("\t%s\n", ipaddr_str);
    }
  }

  freeifaddrs(addresses);
#endif

  return EXIT_SUCCESS;
}
