/*
 * The application of lab3_1.
 * it simply forks a child process.
 */

#include "user/user_lib.h"
#include "util/types.h"

int main(void) {
  uint64 pid = fork();
  if (pid == 0) {
    printu("Child: Hello world!\n");
  } else {
    printu("Parent: Hello world! child id %ld\n", pid);
  }

  exit(0);
}
