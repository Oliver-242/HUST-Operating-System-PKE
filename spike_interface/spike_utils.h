#ifndef _SPIKE_UTILS_H_
#define _SPIKE_UTILS_H_

#include "util/types.h"
#include "spike_file.h"
#include "spike_memory.h"
#include "spike_htif.h"

long frontend_syscall(long n, uint64 a0, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5,
                      uint64 a6);

void poweroff(uint16 code) __attribute((noreturn));
void sprint(const char* s, ...);
void putstring(const char* s);
void shutdown(int) __attribute__((noreturn));

#define assert(x)                              \
  ({                                           \
    if (!(x)) die("assertion failed: %s", #x); \
  })
#define die(str, ...)                                              \
  ({                                                               \
    sprint("%s:%d: " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    poweroff(-1);                                                  \
  })

void do_panic(const char* s, ...) __attribute__((noreturn));
void kassert_fail(const char* s) __attribute__((noreturn));

//void shutdown(int code);

#define panic(s, ...)                \
  do {                               \
    do_panic(s "\n", ##__VA_ARGS__); \
  } while (0)
#define kassert(cond)                    \
  do {                                   \
    if (!(cond)) kassert_fail("" #cond); \
  } while (0)

#endif
