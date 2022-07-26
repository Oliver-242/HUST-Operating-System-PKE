/*
 * The application of lab3_3.
 * parent and child processes never give up their processor during execution.
 */

#include "user/user_lib.h"
#include "util/types.h"

int main(void) {
  uint64 pid = fork();
  uint64 rounds = 100000000;
  uint64 interval = 10000000;
  uint64 a = 0;
  if (pid == 0) {
    printu("Child: Hello world! \n");
    for (uint64 i = 0; i < rounds; ++i) {
      if (i % interval == 0) printu("Child running %ld \n", i);
    }
  } else {
    printu("Parent: Hello world! \n");
    for (uint64 i = 0; i < rounds; ++i) {
      if (i % interval == 0) printu("Parent running %ld \n", i);
    }
  }

  exit(0);
  return 0;
}
