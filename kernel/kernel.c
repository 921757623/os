/*
 * Supervisor-mode startup codes
 */

#include "riscv.h"
#include "string.h"
#include "elf.h"
#include "process.h"

#include "spike_interface/spike_utils.h"

// process is a structure defined in kernel/process.h
process user_app;

//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
void load_user_program(process *proc)
{
  // load_bincode_from_host_elf() is defined in kernel/elf.c
  load_bincode_from_host_elf(proc);
}

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
int s_start(void)
{
  uint64 hartid = read_tp();

  sprint("hartid = %d: Enter supervisor mode...\n", hartid);
  // Note: we use direct (i.e., Bare mode) for memory mapping in lab1.
  // which means: Virtual Address = Physical Address
  // therefore, we need to set satp to be 0 for now. we will enable paging in lab2_x.
  //
  // write_csr is a macro defined in kernel/riscv.h
  write_csr(satp, 0);

  // sprint("the process's addr is %p\n", &user_app);
  //  the application code (elf) is first loaded into memory, and then put into execution
  load_user_program(&user_app);

  sprint("hartid = %d: Switch to user mode...\n", hartid);
  // switch_to() is defined in kernel/process.c
  switch_to(&user_app);

  // we should never reach here.
  return 0;
}
