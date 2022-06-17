#ifndef _MEMLAYOUT_H
#define _MEMLAYOUT_H

// RISC-V machine places its physical memory above DRAM_BASE
#define DRAM_BASE 0x80000000

// the beginning virtual address of PKE kernel
#define KERN_BASE 0x80000000

// default stack size
#define STACK_SIZE 4096

// virtual address of stack top of user process
#define USER_STACK_TOP 0x7ffff000

#endif
