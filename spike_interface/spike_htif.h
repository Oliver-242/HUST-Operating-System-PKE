#ifndef _SPIKE_HTIF_H_
#define _SPIKE_HTIF_H_

#include <stdint.h>
#include "util/types.h"

#if __riscv_xlen == 64
#define TOHOST_CMD(dev, cmd, payload) \
  (((uint64_t)(dev) << 56) | ((uint64_t)(cmd) << 48) | (uint64_t)(payload))
#else
#define TOHOST_CMD(dev, cmd, payload)     \
  ({                                      \
    if ((dev) || (cmd)) __builtin_trap(); \
    (payload);                            \
  })
#endif
#define FROMHOST_DEV(fromhost_value) ((uint64_t)(fromhost_value) >> 56)
#define FROMHOST_CMD(fromhost_value) ((uint64_t)(fromhost_value) << 8 >> 56)
#define FROMHOST_DATA(fromhost_value) ((uint64_t)(fromhost_value) << 16 >> 16)

// HTIF Syscalls
#define HTIFSYS_init_memsize 81
#define HTIFSYS_sema_down 82
#define HTIFSYS_sema_up 83
#define HTIFSYS_exit 93
#define HTIFSYS_exit_group 94
#define HTIFSYS_getpid 172
#define HTIFSYS_kill 129
#define HTIFSYS_read 63
#define HTIFSYS_write 64
#define HTIFSYS_openat 56
#define HTIFSYS_close 57
#define HTIFSYS_lseek 62
#define HTIFSYS_brk 214
#define HTIFSYS_linkat 37
#define HTIFSYS_unlinkat 35
#define HTIFSYS_wait 3
#define HTIFSYS_mkdirat 34
#define HTIFSYS_renameat 38
#define HTIFSYS_chdir 49
#define HTIFSYS_getcwd 17
#define HTIFSYS_fstat 80
#define HTIFSYS_fstatat 79
#define HTIFSYS_faccessat 48
#define HTIFSYS_pread 67
#define HTIFSYS_pwrite 68
#define HTIFSYS_uname 160
#define HTIFSYS_fork 170
#define HTIFSYS_wait 3
#define HTIFSYS_getuid 174
#define HTIFSYS_geteuid 175
#define HTIFSYS_getgid 176
#define HTIFSYS_getegid 177
#define HTIFSYS_mmap 222
#define HTIFSYS_munmap 215
#define HTIFSYS_mremap 216
#define HTIFSYS_mprotect 226
#define HTIFSYS_prlimit64 261
#define HTIFSYS_getmainvars 2011
#define HTIFSYS_rt_sigaction 134
#define HTIFSYS_writev 66
#define HTIFSYS_gettimeofday 169
#define HTIFSYS_times 153
#define HTIFSYS_fcntl 25
#define HTIFSYS_ftruncate 46
#define HTIFSYS_getdents 61
#define HTIFSYS_dup 23
#define HTIFSYS_readlinkat 78
#define HTIFSYS_rt_sigprocmask 135
#define HTIFSYS_ioctl 29
#define HTIFSYS_getrlimit 163
#define HTIFSYS_setrlimit 164
#define HTIFSYS_getrusage 165
#define HTIFSYS_clock_gettime 113
#define HTIFSYS_set_tid_address 96
#define HTIFSYS_set_robust_list 99
#define HTIFSYS_madvise 233

#define HTIFSYS_open 1024
#define HTIFSYS_link 1025
#define HTIFSYS_unlink 1026
#define HTIFSYS_mkdir 1030
#define HTIFSYS_access 1033
#define HTIFSYS_stat 1038
#define HTIFSYS_lstat 1039
#define HTIFSYS_time 1062

#define IS_ERR_VALUE(x) ((unsigned long)(x) >= (unsigned long)-4096)
#define ERR_PTR(x) ((void*)(long)(x))
#define PTR_ERR(x) ((long)(x))

#define AT_FDCWD -100

extern uint64 htif;
void query_htif(uint64 dtb);

// Spike HTIF functionalities
void htif_syscall(uint64);

void htif_console_putchar(uint8_t);
int htif_console_getchar();
void htif_poweroff() __attribute__((noreturn));

#endif
