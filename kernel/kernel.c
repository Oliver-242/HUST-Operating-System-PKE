/*
 * Supervisor-mode startup codes
 */

#include "riscv.h"
#include "string.h"
#include "elf.h"
#include "process.h"
#include "pmm.h"
#include "vmm.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"

// process is a structure defined in kernel/process.h
process user_app;

//
// trap_sec_start points to the beginning of S-mode trap segment (i.e., the entry point of
// S-mode trap vector). added @lab2_1
//
extern char trap_sec_start[];

//
// turn on paging. added @lab2_1
//
void enable_paging() {
  // write the pointer to kernel page (table) directory into the CSR of "satp".
  write_csr(satp, MAKE_SATP(g_kernel_pagetable));

  // refresh tlb to invalidate its content.
  flush_tlb();
}

//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
void load_user_program(process *proc) {
  sprint("User application is loading.\n");
  // allocate a page to store the trapframe. alloc_page is defined in kernel/pmm.c. added @lab2_1
  proc->trapframe = (trapframe *)alloc_page();
  memset(proc->trapframe, 0, sizeof(trapframe));

  // allocate a page to store page directory. added @lab2_1
  proc->pagetable = (pagetable_t)alloc_page();
  memset((void *)proc->pagetable, 0, PGSIZE);

  // allocate pages to both user-kernel stack and user app itself. added @lab2_1
  proc->kstack = (uint64)alloc_page() + PGSIZE;   //user kernel stack top
  uint64 user_stack = (uint64)alloc_page();       //phisical address of user stack bottom

  // USER_STACK_TOP = 0x7ffff000, defined in kernel/memlayout.h
  proc->trapframe->regs.sp = USER_STACK_TOP;  //virtual address of user stack top

  sprint("user frame 0x%lx, user stack 0x%lx, user kstack 0x%lx \n", proc->trapframe,
         proc->trapframe->regs.sp, proc->kstack);

  // load_bincode_from_host_elf() is defined in kernel/elf.c
  load_bincode_from_host_elf(proc);

  // populate the page table of user application. added @lab2_1
  // map user stack in userspace, user_vm_map is defined in kernel/vmm.c
  user_vm_map((pagetable_t)proc->pagetable, USER_STACK_TOP - PGSIZE, PGSIZE, user_stack,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  // map trapframe in user space (direct mapping as in kernel space).
  user_vm_map((pagetable_t)proc->pagetable, (uint64)proc->trapframe, PGSIZE, (uint64)proc->trapframe,
         prot_to_type(PROT_WRITE | PROT_READ, 0));

  // map S-mode trap vector section in user space (direct mapping as in kernel space)
  // here, we assume that the size of usertrap.S is smaller than a page.
  user_vm_map((pagetable_t)proc->pagetable, (uint64)trap_sec_start, PGSIZE, (uint64)trap_sec_start,
         prot_to_type(PROT_READ | PROT_EXEC, 0));
}

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
int s_start(void) {
  sprint("Enter supervisor mode...\n");
  // in the beginning, we use Bare mode (direct) memory mapping as in lab1.
  // but now, we are going to switch to the paging mode @lab2_1.
  // note, the code still works in Bare mode when calling pmm_init() and kern_vm_init().
  write_csr(satp, 0);

  // init phisical memory manager
  pmm_init();

  // build the kernel page table
  kern_vm_init();

  // now, switch to paging mode by turning on paging (SV39)
  enable_paging();
  // the code now formally works in paging mode, meaning the page table is now in use.
  sprint("kernel page table is on \n");

  // the application code (elf) is first loaded into memory, and then put into execution
  load_user_program(&user_app);

  sprint("Switch to user mode...\n");
  // switch_to() is defined in kernel/process.c
  switch_to(&user_app);

  // we should never reach here.
  return 0;
}
