/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#ifndef __UTIL_SEQLOCK_H
#define __UTIL_SEQLOCK_H

/** \file seqlock.h
 *  \brief seqlock (no bus locks for multiple readers)
 *
 *  WARNING:  Please use this on lean objects only.
 *
 *  Generally, things complicated enough to require function calls,
 *  or STL structures are NOT appropriate to lock with spinlocks.
 *  You will probably get faster throughput using boost::mutex.
 *
 *  You MIGHT begin to think about spinlocks to protect data-only
 *  objects (e.g. a few integers, no malloc/free, no fancy libc
 *  calls).
 *
 *  Also note that if what you are protecting is a uint64_t, there
 *  is an atomic 'set_64bit' possible on most modern 'x86 machines.
 *
 *  -----------------------------------------------------------------------
 *
 *  This file is a dumbed down version of sl2.h using less assembler,
 *  so more portable.
 *  - NO rwlock
 *  - NO x86 spinlock (cas locking provided only to help seqlock)
 *  - seqlock
 *
 *  Users should be concerned only with:
 *  
 *  - class SeqLock;
 *  - class ScopedSeqWriteLock;
 *  - class SeqReadLock;
 *
 *  This SeqLock is like the one in the linux kernel.  It is NOT one
 *  of the fancy ones guaranteeing FIFO ordering of write locks.
 *  (This can be provided if there is a need for it).
 *
 *  Seqlock seqlock;
 *  
 *  Readers:
 * \verbatim
 * SeqReadLock srl(seqlock);
 * do
 * {
 * srl.reading();
 * something (copy out the values you need to read);
 * }
 * while( srl.retry() )
 * \endverbatim
 *   
 * Writers:
 * \verbatim
 * {
 * ScopedSeqWriteLock sswl(seqlock);
 * write something
 * }
 * 
 * or
 *
 * {
 * ScopedSeqWriteTryLock lk(slk);
 * if(lk.isLocked())
 * {
 * write something
 * }
 * }
 * \endverbatim
 * 
 *  SeqLock shines when there are only a few writers and an
 *  arbitrary number of readers.  However, in worst case, it's
 *  still faster than boost::mutex on average ---  under high
 *  write contention thread starvation is possible.
 *
 *  There is NO debug provided, intentionally.  Use boost::mutex
 *  if you are still debugging.
 *
 */
// XXX for dbug only

#include <util/atomic.h>
/////////////////////////////////////////////// misc stuff
//
//  Here are some helpers that I had added to private atomic.h
//  on that old 'Speed' branch...
//
//  They are here just so they don't feel homeless.
//
//  Actually, I think seqlock only uses some of the memory barriers,
//  and the set_64bit is not needed anywhere, as far as I know [ejk].
//
///////////////////////////////////////////////


#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif
#ifndef INLINE
#define INLINE inline
#endif

namespace util
{
  // XXX grab the cmpxchg from kernelish.h XXX
  // This one is garbage compared to the flexible one
  // that usually is available to kernel coders.
  //
  //  This only difference with cas32 is that I insist on
  //  the ALWAYS_INLINE.
  ALWAYS_INLINE uint32 __attribute__((visibility ("hidden")))
  cas( volatile uint32 *i, uint32 const oldval, uint32 const newval )
  {
	uint32 prev;

	asm volatile(
				 "\tlock ; cmpxchgl %1, %2"
				 :"=a" (prev)
				 :"r" (newval), "m" (*i), "0" (oldval)
				 :"memory"
				 );
	return prev;
  }

  class SpinLock;             //*< cas-based version, just enough function for SeqLock

  class SeqLock;

  // SeqReadLock rl(slk); do{ rl.reading(); <read-stuff>; } while(rl.retry());
  class SeqReadLock;

  // { ScopedSeqWriteLock lk(slk); <write-stuff>; }
  class ScopedSeqWriteLock;

  // {ScopedSeqWriteTryLock lk(slk); if(lk.isLocked()) {<write-stuff>;}}
  class ScopedSeqWriteTryLock;


  /** Note that all useful ops are private.  Why?
   *
   * SMP safety = volatile + locked ops + memory barriers.
   *
   * This class lacks memory barriers.  It provides the
   * spinlock 'concept' only to trusted friends, that know
   * how and where memory barriers are needed.
   *
   * So all the operations are private and the name is
   * not exported.
   *
   */
  class UnsafeSpinLock 
  {
  private:
	volatile uint_least32_t slock;

  private:
	// I don't need these yet.  You probably want to be
	// sure that copied objects get "unlocked" state, though :)
	UnsafeSpinLock( UnsafeSpinLock const &other );
	UnsafeSpinLock& operator=( UnsafeSpinLock const &other );

	// kernelish asm version removed -- see sl2.h in unittests
	// This requires more asm, while the cas version just
	// reuses existing utilkit cas32 (only) (simpler,
	// maybe slower, but introduces less points of failure.)
	//
	// This version really ought to work -- it did for me
	// several times, but I just saw no big difference in
	// performance between the two versions

	/** A lock/unlock alternate is caslock, casunlock
	 *  It risks being very slightly slower (lots more
	 *  locked instructions going on).
	 *
	 *  It is a bit slower, but not by much.  What it gives is
	 *  easier portability, since now we do "everything" with
	 *  just the CAS operation.
	 */
	INLINE void caslock()
	{
	  //cout<<" caslock "; cout.flush();
	  while( ! cas( &slock, 1U, 0U ) == 1U )
		{
		  cpu_relax();
		}
	}
	INLINE bool castrylock() // added to support ScopedSeqWriteTryLock
	{
	  //cout<<" castrylock "; cout.flush();
	  return cas( &slock, 1U, 0U ) == 1U;
	}
	INLINE void casunlock()
	{
	  //cout<<" casunlock() "; cout.flush();
	  //cas( &slock, 0U, 1U );
	  // NOTE: unlock does not need to be locked operation.
	  asm volatile("movl $1,%0" :"=m" (slock) : :"memory");
	}

  public:
	ALWAYS_INLINE UnsafeSpinLock()
	  : slock(1)
	{}
	ALWAYS_INLINE ~UnsafeSpinLock()
	{}
	//friend class ScopedSpinLock; // I'm not providing this type of lock
	//friend class ScopedCasLock;  // I'm not providing this type of lock
	friend class ScopedSeqWriteLock;
	friend class ScopedSeqWriteTryLock;
  };

  /** This is plain vanilla.  It's probably possible
   *  to write it using a single int value and some trickery.
   *
   *  Read and Write locks are NOT recursive.
   */
  class SeqLock
  {
  private:
	// I don't need these yet.  You probably want to be
	// sure that copied objects get "unlocked" state, though :)
	SeqLock( SeqLock const &other );
	SeqLock& operator=( SeqLock const &other );
  public:
	ALWAYS_INLINE SeqLock()
	  : sl()
	  , seqnum(0U)
	{
	}
	ALWAYS_INLINE ~SeqLock()
	{
	}

  private:
	UnsafeSpinLock sl;	          //*< for the writers
	volatile uint32 seqnum; //*< for the readers

	friend class SeqReadLock; // with start() and end() seqnum getters
	friend class ScopedSeqWriteLock;
	friend class ScopedSeqWriteTryLock;
  };

  /** Writers increment the seqnum after getting the lock
   *  and also just before releasing the lock.
   *
   *  Writers do not block on readers.  They only contend
   *  with other writers.
   */
  class ScopedSeqWriteLock
  {
  public:
	ScopedSeqWriteLock( SeqLock &seqlock )
	  : sql( seqlock )
	{
	  this->sql.sl.caslock();
	  ++this->sql.seqnum;
	  smp_wmb();
	}
	~ScopedSeqWriteLock()
	{
	  smp_wmb();
	  this->sql.seqnum++; // why didn't I pre-increment? Is there something subtle?
	  this->sql.sl.casunlock();
	}

  private:
	SeqLock &sql;
  };

  /** I only support a trivial TryLock API:
   *  
   *  Constructor always tries,
   *  then user must test with isLocked()
   */
  class ScopedSeqWriteTryLock
  {
  private:
	SeqLock &sql;
	bool gotLock;
  public:
	ScopedSeqWriteTryLock( SeqLock &seqlock )
	  : sql( seqlock )
	  , gotLock( seqlock.sl.castrylock() )
	{
	  if( this->gotLock )
		{
		  ++this->sql.seqnum;
		  smp_wmb();
		}
	}
	bool isLocked() const
	{
	  return this->gotLock;
	}
	~ScopedSeqWriteTryLock()
	{
	  if( this->gotLock )
		{
		  smp_wmb();
		  this->sql.seqnum++;
		  this->sql.sl.casunlock();
		}
	}
  };
                    
  /** Readers operate in a retry-if-necessary mode:
   *
   *  SeqReadLock srl(seqlock);
   *  do{
   *    srl.reading();
   *    something;
   *  } while( srl.retry() )
   *
   * \note read memory barrier is a subtle beast.  It not only ensures
   * that previous read complete (as web docs quickly tell you), but
   * also ensures that previous writes get shared as expected between
   * caches.  You can remove some of these rmb's and watch tests sporadically
   * fail  (I think they can actually hang with locking code, which is a great,
   * since otherwise you might not realize that something has been badly
   * read).
   *
   * I suppose, in a way this might still be "read" info sharing, if you
   * think of it as forcing the cache circuitry to properly read 
   * "cache invalidation signals"  that are sent between cache banks
   * associated with various CPUs.  But it "feels" like rmb is making
   * previous writes (on other CPUs) be properly shared as a result.
   *
   * \note: to be tested --- An "atomic_read_rmb" might be better (if provided),
   * since I think the lock prefix might do the rmb automatically on x86 machines.
   *
   * Q: do I need *both* rmb's?
   *
   * Q: does rep_nop influence loop exit time in case you have tons of
   * simultaneous writers?  Maybe rep_nop can have no effect if your are
   * already using memory barriers?
   *
   * \note:
   * This version is OK for my purposes, gaurding loads
   * of a few small variables, with very infrequent writes.
   * If you want a fancier version suitable for highly
   * frequent writers, see sl2.h on private Speed branch.
   */
  class SeqReadLock
  {
  private:
	SeqReadLock();
	SeqReadLock( SeqReadLock const & src );

  public:
	ALWAYS_INLINE SeqReadLock( SeqLock &seqlock )
	  : sql( seqlock )
	  , seq0( sql.seqnum )
	{
	}
	INLINE ~SeqReadLock()
	{
	}


	ALWAYS_INLINE void reading()
	{

	  // This approach, with frequent writes
	  smp_rmb();
	  this->seq0 = sql.seqnum;
	  //......... assembler vaguely like:
	  //movl    inc_seqlock+4, %eax
	  //#APP
	  //  lock; addl $0,0(%esp)
	  //#NO_APP
	}

	ALWAYS_INLINE bool retry() const
	{
	  smp_rmb();
	  // This version is like kernel code.
	  return (this->seq0 & 1U) | (sql.seqnum ^ this->seq0);
	}

  private:
	SeqLock &sql;
	uint32 seq0;
  };

} // util::

#endif // __UTIL_SEQLOCK_H
