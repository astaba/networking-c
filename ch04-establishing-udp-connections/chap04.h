// ch04-establishing-udp-connections/chap04.h
// Preprocessor directives to bridge Windows and UNIX-like systems
#if defined(_WIN32) // If the program is being compiled on Windows

#include <conio.h> // Required by _kbhit()
#include <minwindef.h>
#include <winsock2.h> // Windows Sockets API
#include <ws2tcpip.h> // Extensions for modern IP and DNS support
#if !defined(IPV6_V6ONLY)
#define IPV6_V6ONLY 27
#endif
// To cater for obsolete Windows missing (previous to Vista)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#pragma comment(lib, "ws2_32.lib")
// Platform-specific macros to bridge Windows and UNIX-like systems
// == Windows-specific macros ==================================================
#define GETSOCKETERRNO() (WSAGetLastError())
#define CLOSESOCKET(s) (closesocket(s))
#define BAD_SOCKET(s) (s == INVALID_SOCKET)
#define REPORT_SOCKET_ERROR(context)                                           \
  do {                                                                         \
    int err = WSAGetLastError();                                               \
    char *msg = NULL;                                                          \
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |                            \
                       FORMAT_MESSAGE_FROM_SYSTEM,                             \
                   0, err, 0, (LPSTR) & msg, 0, 0);                            \
    fprintf(stderr, "%s failed: %d (%s)\n", context, err, msg);                \
    LocalFree(msg);                                                            \
  } while (0)

#else

// #include <arpa/inet.h>  // Functions for working with IP addresses
// #include <errno.h>      // Error handling
#include <netdb.h>      // DNS-related functions
// #include <netinet/in.h> // Structures for handling internet addresses
#include <sys/socket.h> // Socket-related functions
// #include <sys/types.h>  // Defines data types used in system calls
#include <sys/select.h>
#include <unistd.h> // UNIX standard functions (e.g., close)
// Platform-specific macros to bridge Windows and UNIX-like systems
// == UNIX-specific macros =====================================================
#define SOCKET int  // Sockets are just file descriptors (int) on UNIX
#define GETSOCKETERRNO() (errno) // Get the last error code on UNIX
#define CLOSESOCKET(s) close(s)  // Close a socket on UNIX
#define BAD_SOCKET(s) (s < 0)    // Check if a socket is valid on UNIX
#define REPORT_SOCKET_ERROR(context) perror(context)

#endif

// Standard C library headers
#include <ctype.h>
#include <stdio.h>  // Standard input/output
#include <stdlib.h> // Standard library functions (e.g., exit)
#include <string.h> // String manipulation
