/*
 * The application of lab2_challenge1_pagefault.
 * Based on application of lab2_3.
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
uint64 sum_sequence(uint64 n, int *p) {
  if (n == 0)
    return 0;
  else
    return *p=sum_sequence( n-1, p+1 ) + n;
}

int main(void) {
  // FIRST, we need a large enough "n" to trigger pagefaults in the user stack
  uint64 n = 1024;

  // alloc a page size array(int) to store the result of every step
  // the max limit of the number is 4kB/4 = 1024

  // SECOND, we use array out of bound to trigger pagefaults in an invalid address
  int *ans = (int *)naive_malloc();

  printu("Summation of an arithmetic sequence from 0 to %ld is: %ld \n", n, sum_sequence(n+1, ans) );

  exit(0);
}
