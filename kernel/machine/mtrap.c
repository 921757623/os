#include "kernel/riscv.h"
#include "kernel/process.h"
#include "spike_interface/spike_utils.h"
#include "spike_interface/spike_file.h"
#include "string.h"

static void handle_instruction_access_fault() { panic("Instruction access fault!"); }

static void handle_load_access_fault() { panic("Load access fault!"); }

static void handle_store_access_fault() { panic("Store/AMO access fault!"); }

static void handle_illegal_instruction() { panic("Illegal instruction!"); }

static void handle_misaligned_load() { panic("Misaligned Load!"); }

static void handle_misaligned_store() { panic("Misaligned AMO!"); }

// added @lab1_3
static void handle_timer()
{
  int cpuid = 0;
  // setup the timer fired at next time (TIMER_INTERVAL from now)
  *(uint64 *)CLINT_MTIMECMP(cpuid) = *(uint64 *)CLINT_MTIMECMP(cpuid) + TIMER_INTERVAL;

  // setup a soft interrupt in sip (S-mode Interrupt Pending) to be handled in S-mode
  write_csr(sip, SIP_SSIP);
}
// 获取路径
char path[128] = {0};
char code[10240] = {0};
struct stat _stat;

static void get_info(addr_line *line)
{
  // 复制路径
  int i = 0;
  while (current->dir[current->file[line->file].dir][i])
  {
    path[i] = current->dir[current->file[line->file].dir][i];
    i++;
  }
  path[i] = '/';
  strcpy(path + strlen(path), current->file[line->file].file);
  // sprint("%s %d\n", path, line->line);

  // 获取代码文本
  spike_file_t *file = spike_file_open(path, O_RDONLY, 0);
  spike_file_stat(file, &_stat);
  spike_file_read(file, code, _stat.st_size);
  spike_file_close(file);

  i = 0;
  int current_line = 0, pre_i = 0;
  while (i < _stat.st_size)
  {
    pre_i = i;
    while (code[i++] != '\n')
      ;
    current_line++;
    if (current_line == line->line)
    {
      code[i - 1] = '\0';
      sprint("Runtime error at %s:%d\n%s\n", path, line->line, code + pre_i);
      break;
    }
  }
}

void print_error_info()
{
  uint64 _mepc = read_csr(mepc);
  // sprint("%p\n", read_csr(mepc));

  for (int i = 0; i < current->line_ind; i++)
  {
    // 找到出错代码的行数，因为current->line的addr是递增的，所以只需要比较下一个的即可
    if (_mepc < current->line[i + 1].addr)
    {
      // sprint("%p %d %d\n", current->line[i].addr, current->line[i].line, current->line[i].file);
      get_info(current->line + i);
      break;
    }
  }
}

//
// handle_mtrap calls a handling function according to the type of a machine mode interrupt (trap).
//
void handle_mtrap()
{
  uint64 mcause = read_csr(mcause);

  if (mcause != CAUSE_MTIMER)
  {
    print_error_info();
  }

  switch (mcause)
  {
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
    panic("unexpected exception happened in M-mode.\n");
    break;
  }
}
