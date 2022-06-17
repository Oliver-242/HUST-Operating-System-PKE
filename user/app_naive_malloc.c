/*
 * Below is the given application for lab2_2.
 */

#include "user_lib.h"
#include "util/types.h"

struct my_structure {
  char c;
  int n;
};

int main(void) {
  struct my_structure* s = (struct my_structure*)naive_malloc();
  s->c = 'a';
  s->n = 1;

  printu("s: %lx, {%c %d}\n", s, s->c, s->n);

  naive_free(s);
  exit(0);
}
