// Mutual exclusion spin locks.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "proc.h"
#include "spinlock.h"

void
initlock(struct spinlock *lk, char *name)
{
  lk->locked = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
acquire(struct spinlock *lk)
{
	// TODO
}

// Release the lock.
void
release(struct spinlock *lk)
{
	// TODO
}
