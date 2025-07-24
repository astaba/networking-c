# GDB

## Tracing recursive functions

Tracing recursive functions with pointers in GDB is where the real power of the debugger shines. Here are the essential GDB commands you'll need to dissect `print_name` and understand its flow:

Assuming you've compiled your test code with `-g` (e.g., `gcc -g -o test_dns_name test_dns_name.c`) and started GDB (`gdb ./test_dns_name`).

### 1\. Basic Navigation

  * **`b print_name`**: Set a breakpoint at the beginning of the `print_name` function.
  * **`r`**: Run the program. It will stop at the first breakpoint.
  * **`n` (next)**: Execute the current line and move to the next. If the current line is a function call, it will execute the function and stop at the line *after* the call (does not step *into* the function).
  * **`s` (step)**: Execute the current line. If the current line is a function call, it will step *into* that function and stop at its first line. This is crucial for tracing recursion.
  * **`fin` (finish)**: Execute the rest of the current function and stop when it returns to its caller.
  * **`c` (continue)**: Continue execution until the next breakpoint or the end of the program.

### 2\. Examining Variables and Memory

  * **`p <variable_name>` (print)**: Print the value of a variable.
      * `p len`: Print the value of `len`.
      * `p *p`: Dereference the pointer `p` and print the byte value it points to.
      * `p k`: Print the calculated offset `k`.
  * **`p/x <variable_name>`**: Print the value in hexadecimal.
      * `p/x *p`: Print the byte at `p` in hex.
      * `p/x k`: Print the offset `k` in hex.
  * **`p/t <variable_name>`**: Print the value in binary (useful for flags).
      * `p/t *p`: Print the byte at `p` in binary.
      * `p/t (*p & 0xC0)`: Print the result of the pointer check in binary.
  * **`x/<N><F><U> <address>` (examine)**: Examine memory at a given address.
      * `<N>`: Number of units to display.

      * `<F>`: Format:

          | Format | Description |
          |--------|-------------|
          | `x` | Hexadecimal |
          | `d` | Decimal |
          | `o` | Octal |
          | `u` | Unsigned decimal |
          | `t` | Binary |
          | `f` | Floating point |
          | `s` | String (null-terminated) |
          | `i` | Instruction (assembly) |

      * `<U>`: Unit size:

          | Unit size | Description |
          |-----------|--------------|
          | `b` | byte |
          | `h` | halfword/2 bytes |
          | `w` | word/4 bytes |
          | `g` | giant word/8 bytes |


      * `x/2xb p`: Examine 2 bytes at address `p` in hex. This is useful for seeing `*p` and `p[1]` together.

      * `x/s p`: Try to interpret memory at `p` as a string (will print until a null terminator). Useful for labels.

      * `x/10xb msg`: Examine the first 10 bytes of the `msg` buffer in hex.

      * `x/hx msg+k`: Examine the 2-byte word (half-word) at `msg+k` in hex. Useful for seeing the actual content a pointer points to.

### 3\. Watching Variable Changes (`watch`)

This is incredibly powerful for recursive functions. GDB will stop execution *immediately* whenever the value of a watched variable changes.

  * **`watch p`**: Stop whenever the `p` pointer changes its value.
  * **`watch *p`**: Stop whenever the value at the memory location `p` points to changes. (Less useful here as `p` advances, not the content at a fixed location).
  * **`watch k`**: Stop whenever the `k` offset changes.

To remove watches: `info watchpoints`, then `d <watchpoint_number>`.

### 4\. Tracking Stack Frames (`bt`)

When you're deep in recursion, this is essential to understand the call hierarchy.

  * **`bt` (backtrace)**: Print the call stack. This shows all the active function calls that led to the current point of execution. Each entry is a "frame."
      * Example output:
        ```
        #0  print_name (msg=0x7ffff... "...", p=0x7ffff... "...", endafter=0x7ffff... "") at test_dns_name.c:17
        #1  0x0000555555555198 in print_name (msg=0x7ffff... "...", p=0x7ffff... "...", endafter=0x7ffff... "") at test_dns_name.c:28
        #2  0x000055555555511b in print_name (msg=0x7ffff... "...", p=0x7ffff... "...", endafter=0x7ffff... "") at test_dns_name.c:13
        #3  0x000055555555523a in main () at test_dns_name.c:66
        ```
        This shows `main` called `print_name` (frame \#2), which called `print_name` again (frame \#1), which called `print_name` a third time (frame \#0, the current one).
  * **`info locals`**: Show local variables in the current frame.
  * **`frame <frame_number>`**: Switch to a specific stack frame.
      * `frame 1`: Switch to the caller of the current `print_name` instance.
      * Once you switch frames, you can `p` variables in *that* frame's scope.
  * **`up`**: Move to the caller's frame (one level up).
  * **`down`**: Move to the callee's frame (one level down).

### 5\. TUI Mode

For a more visual experience:

  * **`layout src`**: Shows the source code panel.
  * **`layout regs`**: Shows registers panel (usually not needed for this).
  * **`layout asm`**: Shows disassembly.
  * **`layout split`**: Shows source and disassembly.
  * **`tui enable`**: Enables TUI (if not already in TUI mode).
  * **`tui disable`**: Disables TUI.

### Example Walkthrough Idea (using Snippet 2 - Pointers)

1.  `b print_name`

2.  `r` (stops at `print_name` for `www.google.com`)

3.  `s` (step into the function)

4.  `p p-msg` (See initial offset, should be 12)

5.  `p *p` (See the first byte, should be 3)

6.  `n` (step over the `if (*p & 0xC0)`)

7.  `p len` (See `len` is 3)

8.  `n` (step past `p++`)

9.  `p p-msg` (See `p` is now 16)

10. `n` (step past `printf("%.*s", len, p);`) - You'll see "www" printed.

11. `n` (step past `p += len;`)

12. `p p-msg` (See `p` is now 19)

13. `p *p` (See `*p` is 6, for "google")

14. `s` (step into the recursive call on line 28)

15. **Now you're in a new `print_name` frame.**

      * `bt` (See the stack frames, `print_name` called `print_name`)
      * `p p-msg` (Should be 19)
      * Continue stepping...

16. Eventually, `www.google.com` will parse and the first call to `print_name` will return.

17. `c` (Continue to the second test case, which has a pointer)

18. `b print_name` (if needed, if you already removed previous breakpoint)

19. `r` (stops at `print_name` for the pointer)

20. `p p-msg` (Should be 32)

21. `p/t (*p & 0xC0)` (See that it's `11000000` or `0xC0`)

22. `s` (step into the `if ((*p & 0xC0) == 0xC0)` block)

23. `p k` (See the calculated offset, should be 16)

24. `n` (step past `p += 2;`)

25. `p p-msg` (See `p` is now 34, advanced past the pointer bytes)

26. `s` (step into the recursive call on line 13: `print_name(msg, msg + k, endafter);`)

27. **Now you're in a new `print_name` frame (for "https://www.google.com/search?q=google.com")**

      * `bt` (See the stack frames, `print_name` called `print_name` again)
      * `p p-msg` (Should be 16, because `msg+k` was passed\!)
      * Continue stepping through the "https://www.google.com/search?q=google.com" parsing.

28. When this recursive call finishes (after printing "https://www.google.com/search?q=google.com" and hitting its null terminator), it will return. Execution will go back to the `print_name` frame that *called* it (the one where the pointer was found).

29. You'll then be back at line 14: `return p;`. The `p` here is still at offset 34 (after the original pointer bytes). This value is returned to `main`.

Using these commands, you'll gain a deep understanding of the recursive flow\! 
