#ifndef _VMM_H_
#define _VMM_H_

#include "riscv.h"

/* --- utility functions for virtual address mapping --- */
int map_pages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm);
// permission codes.
enum VMPermision
{
  PROT_NONE = 0,
  PROT_READ = 1,
  PROT_WRITE = 2,
  PROT_EXEC = 4,
};

typedef struct VM_node
{
  short size;           // 该内存块的大小
  short is_free;        // 该内存块时候被回收了，即是否可以被利用
  uint64 node_addr;     // 当前内存块的地址
  struct VM_node *next; // 指向下一个内存块
} vm_node;

typedef struct heap_manage_t
{
  uint64 size;
  vm_node *start;
  vm_node *end;
} heap_manage;

uint64 prot_to_type(int prot, int user);
pte_t *page_walk(pagetable_t pagetable, uint64 va, int alloc);
uint64 lookup_pa(pagetable_t pagetable, uint64 va);

/* --- kernel page table --- */
// pointer to kernel page directory
extern pagetable_t g_kernel_pagetable;

void kern_vm_map(pagetable_t page_dir, uint64 va, uint64 pa, uint64 sz, int perm);

// Initialize the kernel pagetable
void kern_vm_init(void);

/* --- user page table --- */
void *user_va_to_pa(pagetable_t page_dir, void *va);
void user_vm_map(pagetable_t page_dir, uint64 va, uint64 size, uint64 pa, int perm);
void user_vm_unmap(pagetable_t page_dir, uint64 va, uint64 size, int free);

void enlarge_heap_size(pagetable_t, heap_manage *, uint64);
uint64 user_vm_allocate(pagetable_t, uint64, uint64);
uint64 user_vm_malloc(pagetable_t, heap_manage *, uint64);
void user_vm_free(pagetable_t, uint64 va);

#endif
