#include "../chap05.h"
#include "../dns_query/print_api.h"

int main() {
  // Example: "www.example.com"
  // 3 www 7 example 3 com 0
  unsigned char test_name_buffer[] = {
      3, 'w', 'w', 'w', 7, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3, 'c', 'o', 'm',
      0 // Null terminator
  };

  const unsigned char *msg_start = test_name_buffer;
  // Name starts at the beginning of this buffer
  const unsigned char *name_start = test_name_buffer;
  const unsigned char *buffer_end = test_name_buffer + sizeof(test_name_buffer);

  printf("Parsing name 1: ");
  const unsigned char *end_of_name_ptr =
      print_name(msg_start, name_start, buffer_end);
  printf("\n");
  printf("print_name returned pointer: %td bytes from start\n",
         end_of_name_ptr - msg_start);
  printf(
      "Expected return pointer (after null terminator): %td bytes from start\n",
      sizeof(test_name_buffer));

  // Another example: just "localhost"
  unsigned char test_name_buffer2[] = {
      9, 'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't',
      0 // Null terminator
  };
  printf("\nParsing name 2: ");
  end_of_name_ptr = print_name(test_name_buffer2, test_name_buffer2,
                               test_name_buffer2 + sizeof(test_name_buffer2));
  printf("\n");
  printf("print_name returned pointer: %td bytes from start\n",
         end_of_name_ptr - test_name_buffer2);
  printf(
      "Expected return pointer (after null terminator): %td bytes from start\n",
      sizeof(test_name_buffer2));

  return 0;
}
