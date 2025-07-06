// c_programming\networking\chap01\wsa_init.c
// Description: Small program to show case initialization and cleanup of the
// Winsock API

#include <stdio.h>
#include <winsock2.h>
// WARNING: If you use MinGW as your compiler the pragma is ignored.
// Explicitly tell the compiler to link with: -lws2_32
#pragma comment(lib, "ws2_32.lib")

int main(void) {
  WSADATA WSAData;
  unsigned int wVersionRequested = MAKEWORD(2, 2);
  int wsa_error = WSAStartup(wVersionRequested, &WSAData);

  if (wsa_error) {
    fprintf(stderr, "Failed to initialize Winsock.\n");
    exit(EXIT_FAILURE);
  }

  WSACleanup();
  printf("Ok!\n");

  return EXIT_SUCCESS;
}
