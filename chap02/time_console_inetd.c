/*time_console.c*/

#include <stdio.h>
#include <time.h>

int main() {
  time_t timer;
  time(&timer);
  
  printf("HTTP/1.1 200 OK\r\n");
  printf("Content-Type: text/plain\r\n\r\n");
  printf("Local time is: %s", ctime(&timer));

  return 0;
}
