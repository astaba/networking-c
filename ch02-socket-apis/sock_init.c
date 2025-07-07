// ch02-socket-apis/sock_init.c

#if defined(_WIN32)
#include <minwindef.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>

int main(void) {
#ifdef _WIN32
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsa_error = WSAStartup(wVersionRequested, &WSAData);
  if (wsa_error) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }
#endif

  printf("Ready to use socket API.\n");

#ifdef _WIN32
  WSACleanup();
#endif
  return EXIT_SUCCESS;
}
