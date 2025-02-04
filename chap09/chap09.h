/* chap09.h */

// clang-format off
// Conditional compilation for Windows
#if defined(_WIN32)
    // Define the minimum Windows version to support
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0600
    #endif
    // Include Windows Sockets API
    #include <winsock2.h>
    // Include extensions for modern IP and DNS support
    #include <ws2tcpip.h>
    // Link against the Windows Sockets library
    #pragma comment(lib, "ws2_32.lib")
#else
    // Include system headers for UNIX-like systems
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
#endif

// Conditional compilation for Windows
#if defined(_WIN32)
    #define ISVALIDSOCKET(s) ((s) != INVALID_SOCKET)
    #define CLOSESOCKET(s) closesocket(s)
    #define GETSOCKETERRNO() (WSAGetLastError())
#else
    #define ISVALIDSOCKET(s) ((s) >= 0)
    #define CLOSESOCKET(s) close(s)
    #define GETSOCKETERRNO() (errno)
    #define SOCKET int
#endif

// Include standard C library headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
// Include OpenSSL headers
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
// clang-format on
