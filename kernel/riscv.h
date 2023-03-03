#ifndef _RISCV_H_
#define _RISCV_H_

#include "util/types.h"
#include "config.h"

// fields of mstatus, the Machine mode Status register
#define MSTATUS_MPP_MASK (3L << 11) // previous mode mask
#define MSTATUS_MPP_M (3L << 11)    // machine mode (m-mode)
#define MSTATUS_MPP_S (1L << 11)    // supervisor mode (s-mode)
#define MSTATUS_MPP_U (0L << 11)    // user mode (u-mode)
#define MSTATUS_MIE (1L << 3)       // machine-mode interrupt enable
#define MSTATUS_MPIE (1L << 7)      // preserve MIE bit

// values of mcause, the Machine Cause register
#define IRQ_S_EXT 9                 // s-mode external interrupt
#define IRQ_S_TIMER 5               // s-mode timer interrupt
#define IRQ_S_SOFT 1                // s-mode software interrupt
#define IRQ_M_SOFT 3                // m-mode software interrupt

// fields of mip, the Machine Interrupt Pending register
#define MIP_SEIP (1 << IRQ_S_EXT)   // s-mode external interrupt pending
#define MIP_SSIP (1 << IRQ_S_SOFT)  // s-mode software interrupt pending
#define MIP_STIP (1 << IRQ_S_TIMER) // s-mode timer interrupt pending
#define MIP_MSIP (1 << IRQ_M_SOFT)  // m-mode software interrupt pending

// pysical memory protection choices
#define PMP_R 0x01
#define PMP_W 0x02
#define PMP_X 0x04
#define PMP_A 0x18
#define PMP_L 0x80
#define PMP_SHIFT 2

#define PMP_TOR 0x08
#define PMP_NA4 0x10
#define PMP_NAPOT 0x18

// exceptions
#define CAUSE_MISALIGNED_FETCH 0x0     // Instruction address misaligned
#define CAUSE_FETCH_ACCESS 0x1         // Instruction access fault
#define CAUSE_ILLEGAL_INSTRUCTION 0x2  // Illegal Instruction
#define CAUSE_BREAKPOINT 0x3           // Breakpoint
#define CAUSE_MISALIGNED_LOAD 0x4      // Load address misaligned
#define CAUSE_LOAD_ACCESS 0x5          // Load access fault
#define CAUSE_MISALIGNED_STORE 0x6     // Store/AMO address misaligned
#define CAUSE_STORE_ACCESS 0x7         // Store/AMO access fault
#define CAUSE_USER_ECALL 0x8           // Environment call from U-mode
#define CAUSE_SUPERVISOR_ECALL 0x9     // Environment call from S-mode
#define CAUSE_MACHINE_ECALL 0xb        // Environment call from M-mode
#define CAUSE_FETCH_PAGE_FAULT 0xc     // Instruction page fault
#define CAUSE_LOAD_PAGE_FAULT 0xd      // Load page fault
#define CAUSE_STORE_PAGE_FAULT 0xf     // Store/AMO page fault

// irqs (interrupts). added @lab1_3
#define CAUSE_MTIMER 0x8000000000000007
#define CAUSE_MTIMER_S_TRAP 0x8000000000000001

//Supervisor interrupt-pending register
#define SIP_SSIP (1L << 1)

// core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8 * (hartid))
#define CLINT_MTIME (CLINT + 0xBFF8)  // cycles since boot.

// fields of sstatus, the Supervisor mode Status register
#define SSTATUS_SPP (1L << 8)   // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5)  // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4)  // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)   // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)   // User Interrupt Enable
#define SSTATUS_SUM 0x00040000
#define SSTATUS_FS 0x00006000

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9)  // external
#define SIE_STIE (1L << 5)  // timer
#define SIE_SSIE (1L << 1)  // software

// Machine-mode Interrupt Enable
#define MIE_MEIE (1L << 11)  // external
#define MIE_MTIE (1L << 7)   // timer
#define MIE_MSIE (1L << 3)   // software

#define read_const_csr(reg)              \
  ({                                     \
    unsigned long __tmp;                 \
    asm("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp;                               \
  })

static inline int supports_extension(char ext) {
  return read_const_csr(misa) & (1 << (ext - 'A'));
}

#define read_csr(reg)                             \
  ({                                              \
    unsigned long __tmp;                          \
    asm volatile("csrr %0, " #reg : "=r"(__tmp)); \
    __tmp;                                        \
  })

#define write_csr(reg, val) ({ asm volatile("csrw " #reg ", %0" ::"rK"(val)); })

#define swap_csr(reg, val)                                            \
  ({                                                                  \
    unsigned long __tmp;                                              \
    asm volatile("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "rK"(val)); \
    __tmp;                                                            \
  })

#define set_csr(reg, bit)                                             \
  ({                                                                  \
    unsigned long __tmp;                                              \
    asm volatile("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "rK"(bit)); \
    __tmp;                                                            \
  })

// enable device interrupts
static inline void intr_on(void) { write_csr(sstatus, read_csr(sstatus) | SSTATUS_SIE); }

// disable device interrupts
static inline void intr_off(void) { write_csr(sstatus, read_csr(sstatus) & ~SSTATUS_SIE); }

// are device interrupts enabled?
static inline int is_intr_enable(void) {
  //  uint64 x = r_sstatus();
  uint64 x = read_csr(sstatus);
  return (x & SSTATUS_SIE) != 0;
}

// read sp, the stack pointer
static inline uint64 read_sp(void) {
  uint64 x;
  asm volatile("mv %0, sp" : "=r"(x));
  return x;
}

// read tp, the thread pointer, holding hartid (core number), the index into cpus[].
static inline uint64 read_tp(void) {
  uint64 x;
  asm volatile("mv %0, tp" : "=r"(x));
  return x;
}

// write tp, the thread pointer, holding hartid (core number), the index into cpus[].
static inline void write_tp(uint64 x) { asm volatile("mv tp, %0" : : "r"(x)); }

typedef struct riscv_regs_t {
  /*  0  */ uint64 ra;
  /*  8  */ uint64 sp;
  /*  16 */ uint64 gp;
  /*  24 */ uint64 tp;
  /*  32 */ uint64 t0;
  /*  40 */ uint64 t1;
  /*  48 */ uint64 t2;
  /*  56 */ uint64 s0;
  /*  64 */ uint64 s1;
  /*  72 */ uint64 a0;
  /*  80 */ uint64 a1;
  /*  88 */ uint64 a2;
  /*  96 */ uint64 a3;
  /* 104 */ uint64 a4;
  /* 112 */ uint64 a5;
  /* 120 */ uint64 a6;
  /* 128 */ uint64 a7;
  /* 136 */ uint64 s2;
  /* 144 */ uint64 s3;
  /* 152 */ uint64 s4;
  /* 160 */ uint64 s5;
  /* 168 */ uint64 s6;
  /* 176 */ uint64 s7;
  /* 184 */ uint64 s8;
  /* 192 */ uint64 s9;
  /* 196 */ uint64 s10;
  /* 208 */ uint64 s11;
  /* 216 */ uint64 t3;
  /* 224 */ uint64 t4;
  /* 232 */ uint64 t5;
  /* 240 */ uint64 t6;
}riscv_regs;

#endif
