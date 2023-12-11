/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"

#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char *buf, size_t n)
{
  sprint(buf);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code)
{
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process).
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

int find_closest_function(uint64 return_add)
{
  for (int i = 0; i < symbol_info.cnt; i++)
  {
    // 寻找最近的函数
    if (return_add >= symbol_info.symbols[i].st_value && return_add < symbol_info.symbols[i].st_value + symbol_info.symbols[i].st_size)
    {
      sprint("%s\n", symbol_info.name[i]);
      if (strcmp(symbol_info.name[i], "main") == 0)
        return 0;
      return 1;
    }
  }
  return 0;
}

ssize_t sys_user_print_backtrace(uint64 deep)
{
  // uint64 a = current->trapframe->regs.sp; // 810fff40
  sprint("%x\n", *(uint64 *)(current->trapframe->regs.sp + 32));
  uint64 sp = current->trapframe->regs.sp + 32 + 8;
  for (int i = 0; i < deep; i++)
  {
    if (find_closest_function(*(uint64 *)sp) == 0)
      return 0;
    sp += 16;
  }

  return -1;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7)
{
  switch (a0)
  {
  case SYS_user_print:
    return sys_user_print((const char *)a1, a2);
  case SYS_user_exit:
    return sys_user_exit(a1);
  case SYS_user_print_backtrace:
    return sys_user_print_backtrace(a1);
  default:
    panic("Unknown syscall %ld \n", a0);
  }
}
