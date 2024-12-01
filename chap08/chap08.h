/* omniplat.h */

#if defined(_WIN32) // If compiling on Windows
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Target Windows version is Vista or higher
#endif
#include <winsock2.h> // Windows Sockets API
#include <ws2tcpip.h> // Extensions for modern IP and DNS support
// Microsoft Visual C linking instructions
#pragma comment(lib, "ws2_32.lib")

#else // Then compiling on UNIX-like system (Linux, macOS, etc.)
#include <arpa/inet.h>  // Functions for working with IP addresses
#include <errno.h>      // Error handling
#include <netdb.h>      // DNS-related functions
#include <netinet/in.h> // Structures for handling internet addresses
#include <sys/socket.h> // Socket-related functions
#include <sys/types.h>  // Defines data types used in system calls
#include <unistd.h>     // UNIX-specific functions (e.g., close)
#endif

// Platform-specific macros to bridge Windows and UNIX-like systems
#if defined(_WIN32) // If compiling on Windows
#define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define CLOSESOCKET(s) closesocket(s)
#define GETSOCKETERRNO() (WSAGetLastError()) // Get the last error code

#else // Then compiling on UNIX-like system (Linux, macOS, etc.)
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKET int // Sockets are just file descriptors (int) on UNIX
#define GETSOCKETERRNO() (errno)
#endif

// Standard C library headers
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
