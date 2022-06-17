/*
 * The supporting library for applications.
 * Actually, supporting routines for applications are catalogued as the user 
 * library. we don't do that in PKE to make the relationship between application 
 * and user library more straightforward.
 */

#include "user_lib.h"
#include "util/types.h"
#include "util/snprintf.h"
#include "kernel/syscall.h"

uint64 do_user_call(uint64 sysnum, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6,
                 uint64 a7) {
  int ret;

  // before invoking the syscall, arguments of do_user_call are already loaded into the argument
  // registers (a0-a7) of our (emulated) risc-v machine.
  asm volatile(
      "ecall\n"
      "sw a0, %0"  // returns a 32-bit value
      : "=m"(ret)
      :
      : "memory");

  return ret;
}

//
// printu() supports user/lab1_1_helloworld.c
//
int printu(const char* s, ...) {
  va_list vl;
  va_start(vl, s);

  char out[256];  // fixed buffer size.
  int res = vsnprintf(out, sizeof(out), s, vl);
  va_end(vl);
  const char* buf = out;
  size_t n = res < sizeof(out) ? res : sizeof(out);

  // make a syscall to implement the required functionality.
  return do_user_call(SYS_user_print, (uint64)buf, n, 0, 0, 0, 0, 0);
}

//
// applications need to call exit to quit execution.
//
int exit(int code) {
  return do_user_call(SYS_user_exit, code, 0, 0, 0, 0, 0, 0); 
}

//
// lib call to better_malloc
//
void* better_malloc(int n) {
  return (void*)do_user_call(SYS_user_allocate_page, n, 0, 0, 0, 0, 0, 0);
}

//
// lib call to better_free
//
void better_free(void* va) {
  do_user_call(SYS_user_free_page, (uint64)va, 0, 0, 0, 0, 0, 0);
}
