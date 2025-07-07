// ch02-socket-apis/time_console.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
  printf("Local time is: %s\n", ctime(&(time_t){time(NULL)}));
  return EXIT_SUCCESS;
}
