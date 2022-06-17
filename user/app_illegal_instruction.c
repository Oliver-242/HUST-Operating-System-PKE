/*
 * Below is the given application for lab1_2.
 * This app attempts to issue M-mode instruction in U-mode, and consequently raises an exception.
 */

#include "user_lib.h"
#include "util/types.h"

int main(void) {
  printu("Going to hack the system by running privilege instructions.\n");
  // we are now in U(user)-mode, but the "csrw" instruction requires M-mode privilege.
  // Attempting to execute such instruction will raise illegal instruction exception.
  asm volatile("csrw sscratch, 0");
  exit(0);
}
