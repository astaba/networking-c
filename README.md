# Network programming

## 🔄 Ch03 WSL ↔ Windows Networking Reference

Getting the right **hostname (i.e. local IP address)** from either sides.

### 🧭 Connecting to Windows server

####  From Windows client

> ✅ Using the loopback local IP at `127.0.0.1` or `0.0.0.0` is enough with:

  ```
  localhost <port>
  ```

####  From WSL client

> ✅ To connect from WSL client, WSL `localhost` loops back to WSL the same as Windows' do within Windows. So, to connect to Windows local network from within WSL you must connect to Windows `localhost` which is WSL `gateway` from WSL stand point and is given by:

  - 🔹 **Windows CLI**: check the WSL section in  
    ```
    ipconfig
    ```
  - 🔹 **WSL CLI**: look for the `default` line in  
    ```
    ip route
    ```

---

### 🧭 Connecting to WSL server

####  From WSL client

> ✅ Using the loopback local IP at `127.0.0.1` or `0.0.0.0` is enough with:

  ```
  localhost <port>
  ```

####  From Windows client

- ✅ To connect to WSL `localhost` from Windows, Windows client must connect to WSL local IP address within WSL local network and it is given by the `eth0` section of either of the following commands:

  - 🔹 **WSL CLI**:
    ```
    ip address
    ```
    or  
    ```
    ip route
    ```

---

## DNS Message

### Some free public DNS Servers

| DNS Provider | IPv4 Addresses | IPv6 Addresses|
|--------------|----------------|---------------|
| **Cloudflare 1.1.1.1** | `1.1.1.1` | `2606:4700:4700::1111` |
| | `1.0.0.1` | `2606:4700:4700::1001` |
| **FreeDNS** | `37.235.1.174` | |
| | `37.235.1.177` | |
| **Google Public DNS** | `8.8.8.8` | `2001:4860:4860::8888` |
| | `8.8.4.4` | `2001:4860:4860::8844` |
| **OpenDNS** | `208.67.222.222` | `2620:0:ccc::2` |
| | `208.67.220.220` | `2620:0:ccd::2` |

### DNS Protocol Endianness

**WARNING:** OS little-endian vs DNS Protocol big-endian.

**DNS Protocol specifies high-order byte is sent first.**

As an example: When written to memory the 2-byte size decimal integer `999` convert to `0x03E7` and is subject to OS little-endianness rule specifying least significant bytes occupy lowest memory addresses. As a result `999` is commit to memory as `"memory_cells" = { [0]="E7", [1]="03" }`. And send in the same order `[E7, 03]` accros the network by function like `send()` and `recv()`.

However you should think of the DNS Protocol as a human asking for a number he needs to write down. When you ask the year 2025 you do not expect to hear 5, 2, 0 and 2 as little-endianness demands. DNS Protocol expects to be told 2025 before it writes 2, 0, 5 and then 2. Therefore if you were to send `999` through DNS query you are required to send `[03, E7]`.

That is the purpose of bytes swapping functions like `htons()` and `htonl()` (host to network short/long). They read `999` fro memory as `[E7, 03]` swap it to big-endian and deliver it as `[03, E7]`.

### DNS Message format

---

#### DNS Message Header Section

```code
  ┎───────────────────────────────────────────────────────────────────────────────┒
  ┃                       12-byte Header Format (16 bits * 6)                     ┃
  ┠────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┨
  ┃ 00 ┃ 01 ┃ 02 ┃ 03 ┃ 04 ┃ 05 ┃ 06 ┃ 07 ┃ 08 ┃ 09 ┃ 10 ┃ 11 ┃ 12 ┃ 13 ┃ 14 ┃ 15 ┃
  ┖────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┚
                            Header Section of a DNS Message
  ┎───────────────────────────────────────────────────────────────────────────────┒
  ┃                                       ID                                      ┃
  ┠────┰───────────────────┰────┰────┰────┰────┰──────────────┰───────────────────┨
  ┃ QR ┃       OpCode      ┃ AA ┃ TC ┃ RD ┃ RA ┃       Z      ┃       RCODE       ┃
  ┠────┸───────────────────┸────┸────┸────┸────┸──────────────┸───────────────────┨
  ┃                                    QDCOUNT                                    ┃
  ┠───────────────────────────────────────────────────────────────────────────────┨
  ┃                                    ANCOUNT                                    ┃
  ┠───────────────────────────────────────────────────────────────────────────────┨
  ┃                                    NSCOUNT                                    ┃
  ┠───────────────────────────────────────────────────────────────────────────────┨
  ┃                                    ARCOUNT                                    ┃
  ┖───────────────────────────────────────────────────────────────────────────────┚
```

This table illustrates the standard **DNS Message Header format** as defined in RFC 1035. This 12-byte header is the first part of every DNS message (query or response) and is transmitted in **network byte order (big-endian)**.

Here's a brief explanation of each field:

1.  **ID (00-15 bits / First 2 bytes):**
    * A 16-bit identifier assigned by the program that generates any kind of query. This ID is copied into the corresponding response, allowing the client to match replies to outstanding queries.

2.  **Flags (16-31 bits / Second 2 bytes):**
    * A critical 16-bit field containing various control bits for the DNS operation:
        * **QR (0):** Query (0) or Response (1).
        * **OpCode (1-4):** Kind of query (0 for standard query, 1 for inverse query, 2 for server status request).
        * **AA (5):** Authoritative Answer. Set to 1 if the responding name server is an authority for the domain name in question.
        * **TC (6):** TrunCation. Set to 1 if the message was truncated due to length exceeding the transport's limit.
        * **RD (7):** Recursion Desired. Set to 1 if the querier wants the name server to recursively pursue the query.
        * **RA (8):** Recursion Available. Set to 1 if the name server supports recursive queries.
        * **Z (9-11):** Reserved for future use; must be zero.
        * **RCODE (12-15):** Response Code. Indicates the status of the query (e.g., 0 for No Error, 3 for Name Error).

3.  **QDCOUNT (32-47 bits / Third 2 bytes):**
    * A 16-bit unsigned integer specifying the number of entries in the Question section.

4.  **ANCOUNT (48-63 bits / Fourth 2 bytes):**
    * A 16-bit unsigned integer specifying the number of resource records in the Answer section.

5.  **NSCOUNT (64-79 bits / Fifth 2 bytes):**
    * A 16-bit unsigned integer specifying the number of name server resource records in the Authority section.

6.  **ARCOUNT (80-95 bits / Sixth 2 bytes):**
    * A 16-bit unsigned integer specifying the number of resource records in the Additional records section.

---

#### Complete DNS Query Message

```code
  ┎───────────────────────────────────────────────────────────────────────────────┒
  ┃                                   bit count                                   ┃
  ┠────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┨
  ┃ 00 ┃ 01 ┃ 02 ┃ 03 ┃ 04 ┃ 05 ┃ 06 ┃ 07 ┃ 08 ┃ 09 ┃ 10 ┃ 11 ┃ 12 ┃ 13 ┃ 14 ┃ 15 ┃
  ┖────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┚


                                DNS QUERY MESSAGE
┎───────────────────────────────────────────────────────────────────────────────────┒
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                HEADER                                         ┇ ┃
┃ ┇             12 bytes with its 13 sub-fields and bit flags                     ┇ ┃
┃ ┇                                                                               ┇ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃  Question                                                                         ┃
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                                                               ┇ ┃
┃ ┇                                    QNAME                                      ┇ ┃
┃ ┇                               (variable size)                                 ┇ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                QTYPE (2 bytes)                                ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                QCLASS (2 bytes)                               ┃ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃                                                                                   ┃
┃  ┇  Other whole question sections which each one QNAME, QTYPE and QCLASS are    ┇ ┃
┃  ┇  theorytically repeatable header.QDCOUNT times but rarely done               ┇ ┃
┃  ┇                                                                              ┇ ┃
┖───────────────────────────────────────────────────────────────────────────────────┚

```

---

#### Complete DNS Response Message

```code
  ┎───────────────────────────────────────────────────────────────────────────────┒
  ┃                                   bit count                                   ┃
  ┠────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┰────┨
  ┃ 00 ┃ 01 ┃ 02 ┃ 03 ┃ 04 ┃ 05 ┃ 06 ┃ 07 ┃ 08 ┃ 09 ┃ 10 ┃ 11 ┃ 12 ┃ 13 ┃ 14 ┃ 15 ┃
  ┖────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┸────┚


                                DNS RESPONSE MESSAGE
┎───────────────────────────────────────────────────────────────────────────────────┒
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                HEADER                                         ┇ ┃
┃ ┇             12 bytes with its 13 sub-fields and bit flags                     ┇ ┃
┃ ┇                                                                               ┇ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃  Question                                                                         ┃
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                                                               ┇ ┃
┃ ┇                                    QNAME                                      ┇ ┃
┃ ┇                               (variable size)                                 ┇ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                               QTYPE (2 bytes)                                 ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                               QCLASS (2 bytes)                                ┃ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃                                                                                   ┃
┃  ┇           As many question sections as in the incoming DNS Query             ┇ ┃
┃                                                                                   ┃
┃  A Resource Record either: Answer, Authority or Additional                        ┃
┃ ┎───────────────────────────────────────────────────────────────────────────────┒ ┃
┃ ┇                                                                               ┇ ┃
┃ ┇                                    NAME                                       ┇ ┃
┃ ┇                               (variable size)                                 ┇ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                TYPE (2 bytes)                                 ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                CLASS (2 bytes)                                ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                                 TTL (4 bytes)                                 ┃ ┃
┃ ┃                                                                               ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┃                               RDLENGTH (2 bytes)                              ┃ ┃
┃ ┠───────────────────────────────────────────────────────────────────────────────┨ ┃
┃ ┇                                                                               ┇ ┃
┃ ┇                                   RDATA                                       ┇ ┃
┃ ┇                  (as many bytes as indicated by RDLENGTH)                     ┇ ┃
┃ ┇                                                                               ┇ ┃
┃ ┖───────────────────────────────────────────────────────────────────────────────┚ ┃
┃                                                                                   ┃
┃  ┇              RR sections are repeated as many times as:                      ┇ ┃
┃  ┇              header.ANCOUNT + header.NSCOUNT + header.ARCOUNT                ┇ ┃
┃                                                                                   ┃
┖───────────────────────────────────────────────────────────────────────────────────┚

```

---

### DNS Pseudocode for Mental Model

---

#### DNS Message Header Format

The DNS message always starts with a fixed 12-byte header. This header contains essential control information for the entire message.

---

```JavaScript
// JavaScript pseudocode to foster a mental model of the DNS Message Header Format

const header = { // (6 x 16-bit == 12 bytes)
    // Transaction ID: identifier assigned by the query originator.
    // It is copied into the response to match queries with responses.
    id : 0x0000,         // (16-bit) e.g., 0x1234 for an example

    // Flags: A 16-bit field containing various control bits.
    // Each flag is typically represented by a single bit or a few bits.
    qr     : 0b0,    // (1-bit) Query (0) or Response (1)
    opcode : 0b0000, // (4-bit) Standard Query (0), Inverse Query (1), Status (2), etc.
    aa     : 0b0,    // (1-bit) Authoritative Answer (1 if server is authority for the domain)
    tc     : 0b0,    // (1-bit) TrunCation (1 means message truncated due to length limits. Resend query on TCP)
    rd     : 0b0,    // (1-bit) Recursion Desired (1 if recursive query is requested)
    ra     : 0b0,    // (1-bit) Recursion Available (1 if server supports recursion)
    z      : 0b000,  // (3-bit) Reserved for future use (must be zero)
    rcode  : 0b0000, // (4-bit) Response Code (0=No Error, 1=Format Error, 2=Server Failure, 3=Name Error, etc.)

    // Question Count: Number of entries in the Question section.
    qdcount : 0x0000, // (16-bit)

    // Answer Count: Number of resource records in the Answer section.
    ancount : 0x0000, // (16-bit)

    // Name Server Count: Number of name server resource records in the Authority section.
    nscount : 0x0000, // (16-bit)

    // Additional Records Count: Number of resource records in the Additional records section.
    arcount : 0x0000,  // (16-bit)
}
```

  * **`id`**: A 16-bit transaction identifier. This ID is set by the client in a query and copied unchanged into the server's response. It helps the client match a response to its corresponding query.
  * **`flags`**: A 16-bit field containing various flags that control the behavior and indicate the status of the DNS message.
      * **`qr` (Query/Response)**: 0 for a query, 1 for a response.
      * **`opcode`**: A 4-bit field specifying the type of query. Standard queries are `0`.
      * **`aa` (Authoritative Answer)**: Set to 1 in a response if the responding name server is an authority for the domain name in question.
      * **`tc` (TrunCation)**: Set to 1 if the message was truncated due to length limits (e.g., if a UDP response exceeds 512 bytes). It is a prompt to resend to query on TCP.
      * **`rd` (Recursion Desired)**: Set to 1 in a query if the client wants the name server to perform a recursive query (i.e., resolve the name completely, even if it needs to contact other servers).
      * **`ra` (Recursion Available)**: Set to 1 in a response if the name server supports recursive queries.
      * **`z`**: A 3-bit field reserved for future use; always set to 0.
      * **`rcode` (Response Code)**: A 4-bit field indicating the status of the query. `0` means No Error. Other values indicate various error conditions.
  * **`qdcount` (Question Count)**: A 16-bit unsigned integer specifying the number of entries in the Question section.
  * **`ancount` (Answer Count)**: A 16-bit unsigned integer specifying the number of resource records in the Answer section.
  * **`nscount` (Name Server Count)**: A 16-bit unsigned integer specifying the number of name server resource records in the Authority section.
  * **`arcount` (Additional Records Count)**: A 16-bit unsigned integer specifying the number of resource records in the Additional records section.

-----

#### For the Query:

```JavaScript
// JavaScript pseudocode to foster a mental model of the DNS Query Format

const query = {
    // HEADER
    header : {/* 12-byte 13 fields and bit flags */},

    // The 'QUESTION' part made up of qname, qtype and qclass is theoretically
    // repeatable header.qdcount times, but in practice,
    // almost all DNS clients send queries with only 1 question.

    // QUESTION
    // The domain name being queried, represented as a sequence of
    // length-prefixed labels ending with a null byte (e.g., `3www6google3com0`).
    // No compression pointers in queries and the byte size is variable
    qname : [
        '3', 'w', 'w', 'w',
        '6', 'g', 'o', 'o', 'g', 'l', 'e',
        '3', 'c', 'o', 'm',
        '\0',
    ],

    qtype : 0x0000,    // (2-byte) type of the original query e.g. type:
    // 1 for A (IPv4 address record),
    // 28 for AAAA (IPv6 address record),
    // 15 for MX (mail exchange record),
    // 16 for TXT (Text record),
    // 5 for CNAME (canonical name),
    // 255 for * (any).

    qclass : 0x0000    // (2-byte) class of the original query
    // The class of the data (e.g., 1 for IN - Internet).
}
```

  * **`header`**: This refers to the fixed 12-byte header structure described above.
  * **`question`**: This represents a single question entry. While the DNS specification *allows* for `QDCOUNT` (Question Count in the header) to be greater than 1 (meaning multiple questions in a single query), in practice, almost all DNS clients send queries with only **one** question. It's theoretically possible, but rarely implemented.

-----

#### For the Response:

```JavaScript
// JavaScript pseudocode to foster a mental model of the DNS Response Format

const response = {
    // HEADER
    header : {/* 12-byte 13 fields and bit flags */},

    // follows the same specification as in the query
    question : {/* any-byte qname + 2-byte qtype + 2-byte qclass  */},

    // RESOURCE RECORD
    // The "Resource Record" field acts as a template for entries in the
    // Answer, Authority, and Additional sections repeatable n times:
    // n = header.ancount + header.nscount + header.arcount

    name : {/* if name; then any-bytes; else 2-byte compression pointer */},
    // The domain name to which this resource record pertains.
    // Can be a full name just as the query qname, commonly,
    // a compression pointer (e.g., `0xC0XX`)
    // to an earlier occurrence of the name in the message.

    // The type of resource data
    type : 0x0000,     // (2-byte)

    // The class of the data.
    class : 0x0000,    // (2-byte)

    // Time To Live in seconds. How long the record can be cached.
    ttl : 0x00000000,  // (4-byte)

    // The length of the RDATA field in bytes.
    rdlength : 0x0000, // (2-byte)

    // The actual resource data. Its format depends on the TYPE field
    // (e.g., 4 bytes for an IPv4 address if TYPE is A, another name if TYPE is CNAME or MX).
    rdata : {/* rdlength-bytes */}
}

```

  * **`header`**: The response header will match the query ID, set the QR flag to 1, and contain the counts for each section.
  * **`question`**: A response typically echoes the original query's question section(s).
  * **RESOURCE RECORD**:
      * **"The 'Resource Record' field acts as a template for entries in the Answer, Authority, and Additional sections."**: All three of these sections (Answer, Authority, Additional) are composed of Resource Records (RRs), and each RR follows the `name, type, class, ttl, rdlength, rdata` format. The `print_dns_message` function correctly iterates through `ancount + nscount + arcount` because they all share this common structure.
      * **"These sections are repeatable... Answer: header.ancount times, Authority: header.nscount times, Additional: header.arcount times"**: The `ANCOUNT`, `NSCOUNT`, and `ARCOUNT` fields in the header explicitly tell you how many such `RR` structures to expect in each respective section. Each of these counts can be zero or more.

-----

## Unsafe Re-Entrancy

```c
const char *get_client_address(CLIENTINFO *client) {
  static char address_buffer[100];
  getnameinfo((struct sockaddr *)&client->address, client->address_length,
              address_buffer, sizeof(address_buffer), NULL, 0, NI_NUMERICHOST);
  return address_buffer;
}
```

This `char` array is declared `static`, which ensures that its memory is available after the function returns. This means that we don't need to worry about having the caller `free()` the memory. The downside to this method is because of `get_client_address()` current global state it is not re-entrant-safe:

### Purpose of `static`:

1. **Persistent Storage**:
   ```c
   static char address_buffer[100];
   ```
   The `static` keyword makes `address_buffer` retain its value between function calls. This means that the buffer is allocated only once and persists for the lifetime of the program, rather than being allocated and deallocated each time the function is called.

### How It Relieves the Caller from Calling `free()`:

- **Automatic Memory Management**: Since `address_buffer` is statically allocated, the caller does not need to allocate or free memory for the returned string. The buffer is managed by the function itself, simplifying the caller's code and avoiding potential memory leaks.

### Why It Is Not Re-entrant-safe:

- **Shared State**: The use of a static buffer means that all calls to `get_client_address` share the same buffer. If the function is called concurrently (e.g., from multiple threads), or if it is called again before the previous call's result is used, the buffer's contents can be overwritten, leading to incorrect or unpredictable results.

### Example of Re-entrancy Issue:

Consider the following scenario:

```c
const char *addr1 = get_client_address(client1);
const char *addr2 = get_client_address(client2);
printf("Client 1: %s\n", addr1);
printf("Client 2: %s\n", addr2);
```

In this case, `addr1` and `addr2` will both point to the same `address_buffer`, and the second call to `get_client_address` will overwrite the buffer, causing both `addr1` and `addr2` to contain the address of `client2`.

### Making It Re-entrant-safe:

To make the function re-entrant-safe, you could allocate memory dynamically and let the caller manage it:

```c
char *get_client_address(CLIENTINFO *client) {
    char *address_buffer = malloc(100);
    if (address_buffer == NULL) {
        return NULL; // Handle allocation failure
    }
    getnameinfo((struct sockaddr *)&client->address, client->address_length,
                address_buffer, 100, NULL, 0, NI_NUMERICHOST);
    return address_buffer;
}

// Caller must free the returned buffer
char *address = get_client_address(client);
if (address) {
    printf("Client address: %s\n", address);
    free(address);
}
```

In this version, each call to `get_client_address` returns a new buffer that the caller is responsible for freeing, ensuring that the function is re-entrant-safe.

>**IMPORTANT** The free function does not require the size of the allocated memory to be passed explicitly. The memory allocation system keeps track of the size of each allocated block internally. When you call free, it uses this internal information to correctly deallocate the memory.


## Memory management: Memory leak

### Why is `bind_address` memory freed but not `hints`'s ?

```c
SOCKET create_socket(const char *host, const char *port) {
  // Configure local address the server is listening on
  printf("Configuring local address...\n");
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  getaddrinfo(host, port, &hints, &bind_address);

  // Create socket
  // ...
  // Bind socket to local address
  // ...
  freeaddrinfo(bind_address);
  // Put socket into listening mode
  // ...
  return socket_listen;
}
```

Great question! The reason you need to free the memory for `bind_address` but not for `hints` lies in how memory is allocated and managed for these structures.

1. **`hints` Structure**: This structure is allocated on the stack. When you declare `struct addrinfo hints;`, the memory for `hints` is automatically managed by the system. It is freed when the function `create_socket` returns, so you don't need to explicitly free it.

2. **`bind_address` Structure**: This structure is allocated on the heap by the `getaddrinfo` function. When you call `getaddrinfo`, it dynamically allocates memory for the `bind_address` linked list. Since this memory is allocated on the heap, you must explicitly free it using `freeaddrinfo` to avoid memory leaks¹³.

Here's a brief summary:
- **Stack Allocation**: Automatically managed, no need to free manually (e.g., `hints`).
- **Heap Allocation**: Must be manually freed to avoid memory leaks (e.g., `bind_address`).
