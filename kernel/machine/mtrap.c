#include "kernel/riscv.h"
#include "kernel/process.h"
#include "spike_interface/spike_utils.h"
#include "spike_interface/spike_file.h"
#include <string.h>

char full_path[256];
char full_file[8192];
struct stat f_stat;

void error_printer() {
  uint64 exception_addr = read_csr(mepc);

  for(int i=0; i<current->line_ind; i++) {
    if(exception_addr < current->line[i].addr){        //illegal instruction is on line (i-1)
      addr_line *excpline = current->line + i - 1;

      int dir_len = strlen(current->dir[current->file[excpline->file].dir]);   
      //'**dir' stores dir string, process->line->file points out the index of code_file
      strcpy(full_path, current->dir[current->file[excpline->file].dir]);
      full_path[dir_len] = '/';
      strcpy(full_path+dir_len+1, current->file[excpline->file].file);   
      //filename places after dir/, code_file->file stores the filename
      //sprint(full_path);sprint("%d",excpline->line);sprint("\n");

      //read illegal instruction through spike_file functions
      spike_file_t * _file_ = spike_file_open(full_path, O_RDONLY, 0);
      spike_file_stat(_file_, &f_stat);
      spike_file_read(_file_, full_file, f_stat.st_size);
      spike_file_close(_file_);
      int offset = 0, count = 0;
      while (offset < f_stat.st_size) {
        int temp = offset;
        while (temp < f_stat.st_size && full_file[temp] != '\n') temp++;     //find every line
        if (count == excpline->line - 1) {
        full_file[temp] = '\0';
        sprint("Runtime error at %s:%d\n%s\n", full_path, excpline->line, full_file + offset);
        break;
        } else{
          count++;
          offset = temp + 1;
        }
      }
      break;
    }
  }
}

static void handle_instruction_access_fault() { error_printer(); panic("Instruction access fault!"); }

static void handle_load_access_fault() { error_printer(); panic("Load access fault!"); }

static void handle_store_access_fault() { error_printer(); panic("Store/AMO access fault!"); }

static void handle_illegal_instruction() { error_printer(); panic("Illegal instruction!"); }

static void handle_misaligned_load() { error_printer(); panic("Misaligned Load!"); }

static void handle_misaligned_store() { error_printer(); panic("Misaligned AMO!"); }

// added @lab1_3
static void handle_timer() {
  int cpuid = 0;
  // setup the timer fired at next time (TIMER_INTERVAL from now)
  *(uint64*)CLINT_MTIMECMP(cpuid) = *(uint64*)CLINT_MTIMECMP(cpuid) + TIMER_INTERVAL;

  // setup a soft interrupt in sip (S-mode Interrupt Pending) to be handled in S-mode
  write_csr(sip, SIP_SSIP);
}

//
// handle_mtrap calls a handling function according to the type of a machine mode interrupt (trap).
//
void handle_mtrap() {
  uint64 mcause = read_csr(mcause);
  switch (mcause) {
    case CAUSE_MTIMER:
      handle_timer();
      break;
    case CAUSE_FETCH_ACCESS:
      handle_instruction_access_fault();
      break;
    case CAUSE_LOAD_ACCESS:
      handle_load_access_fault();
    case CAUSE_STORE_ACCESS:
      handle_store_access_fault();
      break;
    case CAUSE_ILLEGAL_INSTRUCTION:
      // TODO (lab1_2): call handle_illegal_instruction to implement illegal instruction
      // interception, and finish lab1_2.
      handle_illegal_instruction();

      break;
    case CAUSE_MISALIGNED_LOAD:
      handle_misaligned_load();
      break;
    case CAUSE_MISALIGNED_STORE:
      handle_misaligned_store();
      break;

    default:
      sprint("machine trap(): unexpected mscause %p\n", mcause);
      sprint("            mepc=%p mtval=%p\n", read_csr(mepc), read_csr(mtval));
      panic( "unexpected exception happened in M-mode.\n" );
      break;
  }
}
