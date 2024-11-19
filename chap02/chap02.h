// Preprocessor directives to bridge Windows and UNIX-like systems
#if defined(_WIN32) // If the program is being compiled on Windows
// To cater for obsolete Windows missing (previous to Vista)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <winsock2.h> // Windows Sockets API
#include <ws2tcpip.h> // Extensions for modern IP and DNS support
#pragma comment(lib, "ws2_32.lib")

#else
#include <arpa/inet.h>  // Functions for working with IP addresses
#include <errno.h>      // Error handling
#include <netdb.h>      // DNS-related functions
#include <netinet/in.h> // Structures for handling internet addresses
#include <sys/socket.h> // Socket-related functions
#include <sys/types.h>  // Defines data types used in system calls
#include <unistd.h>     // UNIX standard functions (e.g., close)
#endif

// Platform-specific macros to bridge Windows and UNIX-like systems
#if defined(_WIN32) // Windows-specific definitions
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s) // Close a socket on Windows
#define GETSOCKETERRNO() (WSAGetLastError())

#else                               // UNIX-specific definitions
#define ISVALIDSOCKET(s) ((s) >= 0) // Check if a socket is valid on UNIX
#define CLOSESOCKET(s) close(s)     // Close a socket on UNIX
#define SOCKET int // Sockets are just file descriptors (int) on UNIX
#define GETSOCKETERRNO() (errno) // Get the last error code on UNIX
#endif

// Standard C library headers
#include <stdio.h>  // Standard input/output
#include <stdlib.h> // Standard library functions (e.g., exit)
#include <string.h> // String manipulation
#include <time.h>   // Time-related functions


