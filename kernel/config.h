#ifndef _CONFIG_H_
#define _CONFIG_H_

// we use only one HART (cpu) in fundamental experiments
#define NCPU 1

#define DRAM_BASE 0x80000000

/* we use fixed physical (also logical) addresses for the stacks and trap frames as in
 Bare memory-mapping mode */
// user stack top
#define USER_STACK 0x81100000

// the stack used by PKE kernel when a syscall happens
#define USER_KSTACK 0x81200000

// the trap frame used to assemble the user "process"
#define USER_TRAP_FRAME 0x81300000

#endif
