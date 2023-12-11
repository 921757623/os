#ifndef _PROC_H_
#define _PROC_H_

#include "riscv.h"

typedef struct trapframe_t
{
  // space to store context (all common registers)
  /* offset:0   */ riscv_regs regs;

  // process's "user kernel" stack
  /* offset:248 */ uint64 kernel_sp;
  // pointer to smode_trap_handler
  /* offset:256 */ uint64 kernel_trap;
  // saved user process counter
  /* offset:264 */ uint64 epc;
} trapframe;

// the extremely simple definition of process, used for begining labs of PKE
typedef struct process_t
{
  // pointing to the stack used in trap handling.
  uint64 kstack;
  // trapframe storing the context of a (User mode) process.
  trapframe *trapframe;
} process;

typedef struct
{
  uint32 st_name; // If the value is nonzero, it represents a string table index that gives the symbol name.
  unsigned char st_info;
  unsigned char st_other;
  uint16 st_shndx;
  uint64 st_value;
  uint64 st_size;
} elf_sym;

typedef struct
{
  elf_sym symbols[32];
  char name[32][32];
  int cnt;
} sym_info;

void switch_to(process *);

extern process *current;
extern sym_info symbol_info;
#endif
