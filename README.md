# Network programming

## DNS

I understand how the interplay of recursion and the `return` statement in this function can be confusing. Let's break it down step by step:

### Overview of the Function
The `print_name` function processes a DNS name, which can either:
1. Be a sequence of labels followed by a zero byte (`0`).
2. Contain a pointer to another part of the DNS message (name compression).

### Key Points of Confusion

1. **Returning `p` after a Pointer (`0xC0`)**:
   - In the pointer-handling block:
     ```c
     if ((*p & 0xC0) == 0xC0) {
         const int k = ((*p & 0x3F) << 8) + p[1];
         p += 2;
         printf(" (pointer %d) ", k);
         print_name(msg, msg + k, end);
         return p;
     }
     ```
     - The pointer is detected and processed (`print_name(msg, msg + k, end);`).
     - **Why is the return value `p`?** The recursion does not return its result; it simply processes the name starting at `msg + k` for output.
     - `p` is returned because the function must continue parsing after the pointer section is done, moving to the next part of the original position in the DNS message.

2. **Returning the Recursive Call in the Non-Pointer Block**:
   - In the non-pointer block:
     ```c
     if (*p) {
         printf(".");
         return print_name(msg, p, end);
     } else {
         return p + 1;
     }
     ```
     - If there is another label (`*p` is non-zero), a `.` is printed, and the function calls itself recursively with `return print_name(msg, p, end);`.
     - This ensures the return value from the deeper recursive call propagates back up the call stack, effectively updating `p` as the parsing progresses.

### Why Some Recursions Are Returned, and Others Are Not
- **Pointer Block**: The recursion (`print_name(msg, msg + k, end)`) is not returned because this is just for printing the dereferenced name. The current position `p` needs to be returned to indicate where parsing should continue after the pointer.
- **Non-Pointer Block**: The recursive call (`return print_name(msg, p, end);`) is returned so that the updated `p` from deeper levels is passed back up. This allows the function to maintain the correct position as it processes subsequent parts of the name.

### Summary
- In the pointer-handling block, recursion is used purely for printing and does not affect the continuation point of `p`, so the function returns `p` after handling the pointer.
- In the non-pointer block, recursion is part of the parsing logic, so the return value is passed back up to maintain the correct parsing position in the DNS message.

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
