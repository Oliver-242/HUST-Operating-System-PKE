/*
 * Below is the given application for lab2_1.
 * This app runs in its own address space, in contrast with in direct mapping.
 */

#include "user_lib.h"
#include "util/types.h"

int main(void) {
  printu("Hello world!\n");
  exit(0);
}
