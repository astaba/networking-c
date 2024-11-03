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
