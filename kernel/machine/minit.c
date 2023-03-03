/*
 * Machine-mode C startup codes
 */

#include "util/types.h"
#include "kernel/riscv.h"
#include "kernel/config.h"
#include "spike_interface/spike_utils.h"

//
// global variables are placed in the .data section.
// stack0 is the privilege mode stack(s) of the proxy kernel on CPU(s)
// allocates 4KB stack space for each processor (hart)
//
// NCPU is defined to be 1 in kernel/config.h, as we consider only one HART in basic
// labs.
//
__attribute__((aligned(16))) char stack0[4096 * NCPU];

// sstart() is the supervisor state entry point defined in kernel/kernel.c
extern void s_start();
// M-mode trap entry point, added @lab1_2
extern void mtrapvec();

// htif is defined in spike_interface/spike_htif.c, marks the availability of HTIF
extern uint64 htif;
// g_mem_size is defined in spike_interface/spike_memory.c, size of the emulated memory
extern uint64 g_mem_size;
// struct riscv_regs is define in kernel/riscv.h, and g_itrframe is used to save
// registers when interrupt hapens in M mode. added @lab1_2
riscv_regs g_itrframe;

//
// get the information of HTIF (calling interface) and the emulated memory by
// parsing the Device Tree Blog (DTB, actually DTS) stored in memory.
//
// the role of DTB is similar to that of Device Address Resolution Table (DART)
// in Intel series CPUs. it records the details of devices and memory of the
// platform simulated using Spike.
//
void init_dtb(uint64 dtb) {
  // defined in spike_interface/spike_htif.c, enabling Host-Target InterFace (HTIF)
  query_htif(dtb);
  if (htif) sprint("HTIF is available!\r\n");

  // defined in spike_interface/spike_memory.c, obtain information about emulated memory
  query_mem(dtb);
  sprint("(Emulated) memory size: %ld MB\n", g_mem_size >> 20);
}

//
// delegate (almost all) interrupts and most exceptions to S-mode.
// after delegation, syscalls will handled by the PKE OS kernel running in S-mode.
//
static void delegate_traps() {
  // supports_extension macro is defined in kernel/riscv.h
  if (!supports_extension('S')) {
    // confirm that our processor supports supervisor mode. abort if it does not.
    sprint("S mode is not supported.\n");
    return;
  }

  // macros used in following two statements are defined in kernel/riscv.h
  uintptr_t interrupts = MIP_SSIP | MIP_STIP | MIP_SEIP;
  uintptr_t exceptions = (1U << CAUSE_MISALIGNED_FETCH) | (1U << CAUSE_FETCH_PAGE_FAULT) |
                         (1U << CAUSE_BREAKPOINT) | (1U << CAUSE_LOAD_PAGE_FAULT) |
                         (1U << CAUSE_STORE_PAGE_FAULT) | (1U << CAUSE_USER_ECALL);

  // writes 64-bit values (interrupts and exceptions) to 'mideleg' and 'medeleg' (two
  // priviledged registers of RV64G machine) respectively.
  //
  // write_csr and read_csr are macros defined in kernel/riscv.h
  write_csr(mideleg, interrupts);
  write_csr(medeleg, exceptions);
  assert(read_csr(mideleg) == interrupts);
  assert(read_csr(medeleg) == exceptions);
}

//
// enabling timer interrupt (irq) in Machine mode. added @lab1_3
//
void timerinit(uintptr_t hartid) {
  // fire timer irq after TIMER_INTERVAL from now.
  *(uint64*)CLINT_MTIMECMP(hartid) = *(uint64*)CLINT_MTIME + TIMER_INTERVAL;

  // enable machine-mode timer irq in MIE (Machine Interrupt Enable) csr.
  write_csr(mie, read_csr(mie) | MIE_MTIE);
}

//
// m_start: machine mode C entry point.
//
void m_start(uintptr_t hartid, uintptr_t dtb) {
  // init the spike file interface (stdin,stdout,stderr)
  // functions with "spike_" prefix are all defined in codes under spike_interface/,
  // sprint is also defined in spike_interface/spike_utils.c
  spike_file_init();
  sprint("In m_start, hartid:%d\n", hartid);

  // init HTIF (Host-Target InterFace) and memory by using the Device Table Blob (DTB)
  // init_dtb() is defined above.
  init_dtb(dtb);

  // save the address of trap frame for interrupt in M mode to "mscratch". added @lab1_2
  write_csr(mscratch, &g_itrframe);

  // set previous privilege mode to S (Supervisor), and will enter S mode after 'mret'
  // write_csr is a macro defined in kernel/riscv.h
  write_csr(mstatus, ((read_csr(mstatus) & ~MSTATUS_MPP_MASK) | MSTATUS_MPP_S));

  // set M Exception Program Counter to sstart, for mret (requires gcc -mcmodel=medany)
  write_csr(mepc, (uint64)s_start);

  // setup trap handling vector for machine mode. added @lab1_2
  write_csr(mtvec, (uint64)mtrapvec);

  // enable machine-mode interrupts. added @lab1_3
  write_csr(mstatus, read_csr(mstatus) | MSTATUS_MIE);

  // delegate all interrupts and exceptions to supervisor mode.
  // delegate_traps() is defined above.
  delegate_traps();

  // also enables interrupt handling in supervisor mode. added @lab1_3
  write_csr(sie, read_csr(sie) | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // init timing. added @lab1_3
  timerinit(hartid);

  // switch to supervisor mode (S mode) and jump to s_start(), i.e., set pc to mepc
  asm volatile("mret");
}
