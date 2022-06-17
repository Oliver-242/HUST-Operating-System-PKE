// See LICENSE for license details.
// borrowed from https://github.com/riscv/riscv-pk:
// machine/atomic.h

#ifndef _RISCV_ATOMIC_H_
#define _RISCV_ATOMIC_H_

// Currently, interrupts are always disabled in M-mode.
// todo: for PKE, wo turn on irq in lab_1_3_timer, so wo have to implement these two functions.
#define disable_irqsave() (0)
#define enable_irqrestore(flags) ((void)(flags))

typedef struct {
  int lock;
  // For debugging:
  char* name;       // Name of lock.
  struct cpu* cpu;  // The cpu holding the lock.
} spinlock_t;

#define SPINLOCK_INIT \
  { 0 }

#define mb() asm volatile("fence" ::: "memory")
#define atomic_set(ptr, val) (*(volatile typeof(*(ptr))*)(ptr) = val)
#define atomic_read(ptr) (*(volatile typeof(*(ptr))*)(ptr))

#define atomic_binop(ptr, inc, op)         \
  ({                                       \
    long flags = disable_irqsave();        \
    typeof(*(ptr)) res = atomic_read(ptr); \
    atomic_set(ptr, op);                   \
    enable_irqrestore(flags);              \
    res;                                   \
  })
#define atomic_add(ptr, inc) atomic_binop(ptr, inc, res + (inc))
#define atomic_or(ptr, inc) atomic_binop(ptr, inc, res | (inc))
#define atomic_swap(ptr, inc) atomic_binop(ptr, inc, (inc))
#define atomic_cas(ptr, cmp, swp)                           \
  ({                                                        \
    long flags = disable_irqsave();                         \
    typeof(*(ptr)) res = *(volatile typeof(*(ptr))*)(ptr);  \
    if (res == (cmp)) *(volatile typeof(ptr))(ptr) = (swp); \
    enable_irqrestore(flags);                               \
    res;                                                    \
  })

static inline int spinlock_trylock(spinlock_t* lock) {
  int res = atomic_swap(&lock->lock, -1);
  mb();
  return res;
}

static inline void spinlock_lock(spinlock_t* lock) {
  do {
    while (atomic_read(&lock->lock))
      ;
  } while (spinlock_trylock(lock));
}

static inline void spinlock_unlock(spinlock_t* lock) {
  mb();
  atomic_set(&lock->lock, 0);
}

static inline long spinlock_lock_irqsave(spinlock_t* lock) {
  long flags = disable_irqsave();
  spinlock_lock(lock);
  return flags;
}

static inline void spinlock_unlock_irqrestore(spinlock_t* lock, long flags) {
  spinlock_unlock(lock);
  enable_irqrestore(flags);
}

#endif
