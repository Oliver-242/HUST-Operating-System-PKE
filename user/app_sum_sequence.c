/*
 * The application of lab2_3.
 */

#include "user_lib.h"
#include "util/types.h"

//
// compute the summation of an arithmetic sequence. for a given "n", compute
// result = n + (n-1) + (n-2) + ... + 0
// sum_sequence() calls itself recursively till 0. The recursive call, however,
// may consume more memory (from stack) than a physical 4KB page, leading to a page fault.
// PKE kernel needs to improved to handle such page fault by expanding the stack.
//
uint64 sum_sequence(uint64 n) {
  if (n == 0)
    return 0;
  else
    return sum_sequence( n-1 ) + n;
}

int main(void) {
  // we need a large enough "n" to trigger pagefaults in the user stack
  uint64 n = 1000;

  printu("Summation of an arithmetic sequence from 0 to %ld is: %ld \n", n, sum_sequence(1000) );
  exit(0);
}
