#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "types.h"

// Mutual exclusion lock.
struct spinlock {
    uint32_t        locked;     // Is the lock held?
    // For debugging:
    const char  *name;      // Name of lock.
   // struct cpu  *cpu;       // The cpu holding the lock.
    uintptr_t   pcs[10];    // The call stack (an array of program counters)
    // that locked the lock.
};

// spinlock.c
void	acquire(struct spinlock*);

void	initlock(struct spinlock*, const char*);
void	release(struct spinlock*);
// Check whether this cpu is holding the lock.
//int             holding(struct spinlock*);
#define 	holding(LK) ((LK)->locked)


#endif
