/*
 * Utility functions for process management. 
 *
 * Note: in Lab1, only one process (i.e., our user application) exists. Therefore, 
 * PKE OS at this stage will set "current" to the loaded user application, and also
 * switch to the old "current" process after trap handling.
 */

#include "riscv.h"
#include "strap.h"
#include "config.h"
#include "process.h"
#include "elf.h"
#include "string.h"
#include "vmm.h"
#include "pmm.h"
#include "memlayout.h"
#include "sched.h"
#include "spike_interface/spike_utils.h"

//Two functions defined in kernel/usertrap.S
extern char smode_trap_vector[];
extern void return_to_user(trapframe *, uint64 satp);

// trap_sec_start points to the beginning of S-mode trap segment (i.e., the entry point
// of S-mode trap vector).
extern char trap_sec_start[];

// process pool. added @lab3_1
process procs[NPROC];

// current points to the currently running user-mode application.
process* current = NULL;

// points to the first free page in our simple heap. added @lab2_2
uint64 g_ufree_page = USER_FREE_ADDRESS_START;

process* blocked_queue_head = NULL;

//
// switch to a user-mode process
//
void switch_to(process* proc) {
  assert(proc);
  current = proc;

  // write the smode_trap_vector (64-bit func. address) defined in kernel/strap_vector.S
  // to the stvec privilege register, such that trap handler pointed by smode_trap_vector
  // will be triggered when an interrupt occurs in S mode.
  write_csr(stvec, (uint64)smode_trap_vector);

  // set up trapframe values (in process structure) that smode_trap_vector will need when
  // the process next re-enters the kernel.
  proc->trapframe->kernel_sp = proc->kstack;      // process's kernel stack
  proc->trapframe->kernel_satp = read_csr(satp);  // kernel page table
  proc->trapframe->kernel_trap = (uint64)smode_trap_handler;

  // SSTATUS_SPP and SSTATUS_SPIE are defined in kernel/riscv.h
  // set S Previous Privilege mode (the SSTATUS_SPP bit in sstatus register) to User mode.
  unsigned long x = read_csr(sstatus);
  x &= ~SSTATUS_SPP;  // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE;  // enable interrupts in user mode

  // write x back to 'sstatus' register to enable interrupts, and sret destination mode.
  write_csr(sstatus, x);

  // set S Exception Program Counter (sepc register) to the elf entry pc.
  write_csr(sepc, proc->trapframe->epc);

  // make user page table. macro MAKE_SATP is defined in kernel/riscv.h. added @lab2_1
  uint64 user_satp = MAKE_SATP(proc->pagetable);

  // return_to_user() is defined in kernel/strap_vector.S. switch to user mode with sret.
  // note, return_to_user takes two parameters @ and after lab2_1.
  return_to_user(proc->trapframe, user_satp);
}

//
// initialize process pool (the procs[] array). added @lab3_1
//
void init_proc_pool() {
  memset( procs, 0, sizeof(process)*NPROC );

  for (int i = 0; i < NPROC; ++i) {
    procs[i].status = FREE;
    procs[i].pid = i;
  }
}

//
// allocate an empty process, init its vm space. returns the pointer to
// process strcuture. added @lab3_1
//
process* alloc_process() {
  // locate the first usable process structure
  int i;

  for( i=0; i<NPROC; i++ )
    if( procs[i].status == FREE ) break;

  if( i>=NPROC ){
    panic( "cannot find any free process structure.\n" );
    return 0;
  }

  // init proc[i]'s vm space
  procs[i].trapframe = (trapframe *)alloc_page();  //trapframe, used to save context
  memset(procs[i].trapframe, 0, sizeof(trapframe));

  // page directory
  procs[i].pagetable = (pagetable_t)alloc_page();
  memset((void *)procs[i].pagetable, 0, PGSIZE);

  procs[i].kstack = (uint64)alloc_page() + PGSIZE;   //user kernel stack top
  uint64 user_stack = (uint64)alloc_page();       //phisical address of user stack bottom
  procs[i].trapframe->regs.sp = USER_STACK_TOP;  //virtual address of user stack top

  // allocates a page to record memory regions (segments)
  procs[i].mapped_info = (mapped_region*)alloc_page();
  memset( procs[i].mapped_info, 0, PGSIZE );

  // map user stack in userspace
  user_vm_map((pagetable_t)procs[i].pagetable, USER_STACK_TOP - PGSIZE, PGSIZE,
    user_stack, prot_to_type(PROT_WRITE | PROT_READ, 1));
  procs[i].mapped_info[0].va = USER_STACK_TOP - PGSIZE;
  procs[i].mapped_info[0].npages = 1;
  procs[i].mapped_info[0].seg_type = STACK_SEGMENT;

  // map trapframe in user space (direct mapping as in kernel space).
  user_vm_map((pagetable_t)procs[i].pagetable, (uint64)procs[i].trapframe, PGSIZE,
    (uint64)procs[i].trapframe, prot_to_type(PROT_WRITE | PROT_READ, 0));
  procs[i].mapped_info[1].va = (uint64)procs[i].trapframe;
  procs[i].mapped_info[1].npages = 1;
  procs[i].mapped_info[1].seg_type = CONTEXT_SEGMENT;

  // map S-mode trap vector section in user space (direct mapping as in kernel space)
  // we assume that the size of usertrap.S is smaller than a page.
  user_vm_map((pagetable_t)procs[i].pagetable, (uint64)trap_sec_start, PGSIZE,
    (uint64)trap_sec_start, prot_to_type(PROT_READ | PROT_EXEC, 0));
  procs[i].mapped_info[2].va = (uint64)trap_sec_start;
  procs[i].mapped_info[2].npages = 1;
  procs[i].mapped_info[2].seg_type = SYSTEM_SEGMENT;

  sprint("in alloc_proc. user frame 0x%lx, user stack 0x%lx, user kstack 0x%lx \n",
    procs[i].trapframe, procs[i].trapframe->regs.sp, procs[i].kstack);

  procs[i].total_mapped_region = 3;
  // return after initialization.
  return &procs[i];
}

//
// reclaim a process. added @lab3_1
//
int free_process( process* proc ) {
  // we set the status to ZOMBIE, but cannot destruct its vm space immediately.
  // since proc can be current process, and its user kernel stack is currently in use!
  // but for proxy kernel, it (memory leaking) may NOT be a really serious issue,
  // as it is different from regular OS, which needs to run 7x24.
  proc->status = ZOMBIE;

  return 0;
}

//
// implements fork syscal in kernel. added @lab3_1
// basic idea here is to first allocate an empty process (child), then duplicate the
// context and data segments of parent process to the child, and lastly, map other
// segments (code, system) of the parent to child. the stack segment remains unchanged
// for the child.
//
int do_fork( process* parent)
{
  sprint( "will fork a child from parent %d.\n", parent->pid );
  process* child = alloc_process();

  for( int i=0; i<parent->total_mapped_region; i++ ){
    // browse parent's vm space, and copy its trapframe and data segments,
    // map its code segment.
    switch( parent->mapped_info[i].seg_type ){
      case CONTEXT_SEGMENT:
        *child->trapframe = *parent->trapframe;
        break;
      case STACK_SEGMENT:
        memcpy( (void*)lookup_pa(child->pagetable, child->mapped_info[0].va),
          (void*)lookup_pa(parent->pagetable, parent->mapped_info[i].va), PGSIZE );
        break;
      case CODE_SEGMENT:
        // TODO (lab3_1): implment the mapping of child code segment to parent's
        // code segment.
        // hint: the virtual address mapping of code segment is tracked in mapped_info
        // page of parent's process structure. use the information in mapped_info to
        // retrieve the virtual to physical mapping of code segment.
        // after having the mapping information, just map the corresponding virtual 
        // address region of child to the physical pages that actually store the code
        // segment of parent process. 
        // DO NOT COPY THE PHYSICAL PAGES, JUST MAP THEM.
        for( int j=0; j<parent->mapped_info[i].npages; j++ ){
            uint64 addr = lookup_pa(parent->pagetable, parent->mapped_info[i].va+j*PGSIZE);

            map_pages(child->pagetable, parent->mapped_info[i].va+j*PGSIZE, PGSIZE,
                    addr, prot_to_type(PROT_WRITE | PROT_READ | PROT_EXEC, 1));

            sprint( "do_fork map code segment at pa:%lx of parent to child at va:%lx.\n",
                    addr, parent->mapped_info[i].va+j*PGSIZE );
        }

        // after mapping, register the vm region (do not delete codes below!)
        child->mapped_info[child->total_mapped_region].va = parent->mapped_info[i].va;
        child->mapped_info[child->total_mapped_region].npages = 
          parent->mapped_info[i].npages;
        child->mapped_info[child->total_mapped_region].seg_type = CODE_SEGMENT;
        child->total_mapped_region++;
        break;
      case DATA_SEGMENT:
        for( int j=0; j<parent->mapped_info[i].npages; j++ ){
            uint64 addr = lookup_pa(parent->pagetable, parent->mapped_info[i].va+j*PGSIZE);
            char *newaddr = alloc_page(); memcpy(newaddr, (void *)addr, PGSIZE);
            map_pages(child->pagetable, parent->mapped_info[i].va+j*PGSIZE, PGSIZE,
                    (uint64)newaddr, prot_to_type(PROT_WRITE | PROT_READ, 1));
        }

        // after mapping, register the vm region (do not delete codes below!)
        child->mapped_info[child->total_mapped_region].va = parent->mapped_info[i].va;
        child->mapped_info[child->total_mapped_region].npages = 
          parent->mapped_info[i].npages;
        child->mapped_info[child->total_mapped_region].seg_type = DATA_SEGMENT;
        child->total_mapped_region++;
        break;
    }
  }

  child->status = READY;
  child->trapframe->regs.a0 = 0;
  child->parent = parent;
  insert_to_ready_queue( child );

  return child->pid;
}

int do_wait(int pid)
{
  int found = 0;
  if (pid == -1) {
    for (int i = 0; i < NPROC; i++)
      if (procs[i].parent == current) {
        found = 1;
          if (procs[i].status == ZOMBIE) {
          procs[i].status = FREE;
          return i;
          }
      }
    if (found == 0) return -1;   //current parent process doesn't have child process. invalid!
    else {
      insert_to_blocked_queue(current);
      schedule();
      return -2;
    }     //there exists a child process without ZOMBIE status
  }
  else if (pid < NPROC) {     //a possibly valid specified child process
    if (procs[pid].parent != current) return -1;//child process with input pid isn't current parent process's child
    else {
      if (procs[pid].status == ZOMBIE) {
        procs[pid].status = FREE;
        return pid;
      }
      else {
        insert_to_blocked_queue(current);
        schedule();
        return -2;
    }  
    }
  }
  else return -1;   //invalid inputs
}

void insert_to_blocked_queue(process *proc) 
{
  if( blocked_queue_head == NULL ){
    proc->status = BLOCKED;
    proc->queue_next = NULL;
    blocked_queue_head = proc;
    return;
  }
  // blocked queue is not empty
  process *p;
  // browse the blocked queue to see if proc is already in-queue
  for( p=blocked_queue_head; p->queue_next!=NULL; p=p->queue_next )
    if( p == proc ) return;  //already in queue

  // p points to the last element of the blocked queue
  if( p==proc ) return;

  p->queue_next = proc;
  proc->status = BLOCKED;
  proc->queue_next = NULL;
  return;
}

void remove_and_insert(process* proc)
{
  process* wakeup = NULL;
  process* p;
  if(blocked_queue_head == NULL) return;
  if(blocked_queue_head == proc->parent){
    wakeup = blocked_queue_head;
    blocked_queue_head = blocked_queue_head->queue_next;
    wakeup->status = READY;
    insert_to_ready_queue(wakeup);
    return;
  }
  for( p=blocked_queue_head; p->queue_next!=NULL; p=p->queue_next )
    if( p->queue_next == proc->parent ) {
      wakeup = p->queue_next;
      p->queue_next = p->queue_next->queue_next;
      wakeup->status = READY;
      insert_to_ready_queue(wakeup);
      return;
    }
}
