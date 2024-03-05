#ifndef _SYNC_UTILS_H_
#define _SYNC_UTILS_H_

#include <stdatomic.h>

typedef struct
{
  atomic_int lock;
} Mutex;

static inline void sync_barrier(volatile int *counter, int all)
{

  int local;

  asm volatile("amoadd.w %0, %2, (%1)\n"
               : "=r"(local)
               : "r"(counter), "r"(1)
               : "memory");

  if (local + 1 < all)
  {
    do
    {
      asm volatile("lw %0, (%1)\n" : "=r"(local) : "r"(counter) : "memory");
    } while (local < all);
  }
}

static inline void init_mutex(Mutex *mutex)
{
  atomic_init(&mutex->lock, 0);
}

static inline void lock_mutex(Mutex *mutex)
{
  int local;

  do
  {
    // 使用amoswap指令尝试获取锁
    asm volatile("amoswap.w %0, %1, (%2)\n"
                 : "=r"(local)
                 : "r"(1), "r"(&mutex->lock)
                 : "memory");
  } while (local != 0);
}

static inline void unlock_mutex(Mutex *mutex)
{
  // 释放锁
  atomic_store(&mutex->lock, 0);
}

#endif