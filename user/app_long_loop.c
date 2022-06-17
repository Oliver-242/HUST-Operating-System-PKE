/*
 * Below is the given application for lab1_3.
 * This app performs a long loop, during which, timers are
 * generated and pop messages to our screen.
 */

#include "user_lib.h"
#include "util/types.h"

int main(void) {
  printu("Hello world!\n");
  int i;
  for (i = 0; i < 100000000; ++i) {
    if (i % 5000000 == 0) printu("wait %d\n", i);
  }

  exit(0);

  return 0;
}
