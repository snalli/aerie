/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef UTIL_ATOMIC_H
#define UTIL_ATOMIC_H

#include <util/types.h>
                     
namespace util
{
  /**
   * 32-bit Compare-And-Swap atomic operation. Implemented
   * using assembly inline code as inline functions
   * (no function call overhead).
   *
   * Note, that the parameters order is like in "classic" CAS,
   * not as in Interlocked*** Microsoft functions.
   *
   * @param p      Pointer to memory destinaiton.
   * @param expVal Expected value (comperand).
   * @param newVal New value (to be exchanged).
   * @return The initial value at the memory destination/
   */
    
  inline uint32 cas32(uint32 volatile* p, uint32 expVal, uint32 newVal)
  {
	uint32 old;
    
	__asm__ __volatile__ ("lock; cmpxchgl %2, %0"
						  : "=m" (*p), "=a" (old)
						  : "r" (newVal), "m" (*p), "a" (expVal)
						  : "memory");
    	
	return old;
  }


  inline void atomicIncrement(uint64 volatile* p)
  {
	__asm__ __volatile__(
						 "lock; incq %0"
						 :"=m" (*p)
						 :"m" (*p)
						 :"memory");
  }

  inline void atomicDecrement(uint64 volatile* p)
  {
	__asm__ __volatile__(
						 "lock; decq %0"
						 :"=m" (*p)
						 :"m" (*p)
						 :"memory");
  }

  inline void atomicIncrement(uint32 volatile* p)
  {
	__asm__ __volatile__(
						 "lock; incl %0"
						 :"=m" (*p)
						 :"m" (*p)
						 :"memory");
  }

  inline void atomicDecrement(uint32 volatile* p)
  {
	__asm__ __volatile__(
						 "lock; decl %0"
						 :"=m" (*p)
						 :"m" (*p)
						 :"memory");
  }

  inline int32 atomicSubAndTest(int32 d, uint32 volatile* p)
  {
	uchar c;
	__asm__ __volatile__(
						 "lock; subl %2,%0; sete %1"
						 :"=m" (*p), "=qm" (c)
						 :"ir" (d), "m" (*p)
						 :"memory");
	return c;
  }

  inline int64 atomicSubAndTest(int64 d, uint64 volatile* p)
  {
	uchar c;
	__asm__ __volatile__(
						 "lock; subq %2,%0; sete %1"
						 :"=m" (*p), "=qm" (c)
						 :"ir" (d), "m" (*p)
						 :"memory");
	return c;
  }
  /*
   * Add i to *p and return (*p + i) atomically
   */
  inline int32 atomicAddReturn(int32 i, int32 volatile* p)
  {
	int32 __i;
	__i = i;
	__asm__ __volatile__(
						 "lock; xaddl %0, %1;"
						 :"=r"(i), "=m"(*p)
						 :"m"(*p), "0"(i)
						 :"memory");
	return i + __i;
  }

  inline int64 atomicAddReturn(int64 i, int64 volatile* p)
  {
	int64 __i;
	__i = i;
	__asm__ __volatile__(
						 "lock; xaddq %0, %1;"
						 :"=r"(i), "=m"(*p)
						 :"m"(*p), "0"(i)
						 :"memory");
	return i + __i;
  }
  /*
   * Subtract i from *p and return (*p - i) atomically
   */
  inline int32 atomicSubReturn(int32 i, int32 volatile* p)
  {
	return atomicAddReturn(-i, p);
  }

  inline int64 atomicSubReturn(int64 i, int64 volatile* p)
  {
	return atomicAddReturn(-i, p);
  }

  /**
   * atomic add and sub for unsigned long values. Simplyfied(possibly faster) version of above functions.
   * NOTE: that (unsigned long) have variable size on different platforms(it's pointer size)
   * following code (addl/subl) is for 32bit(4bytes) long
   * if ulong is 64bit, please change (addl/subl) to (addq/subq)
   */
  //static CTAssert< sizeof(unsigned long) == 4 > reqire64bitAtomics __attribute__((unused));
  inline void atomicAdd(uint32 i, uint32 volatile* p)
  {
	__asm__ __volatile__(
						 "lock; addl %1, %0;"
						 :"=m"(*p)
						 :"ir"(i), "m"(*p)
						 :"memory");
  }

  inline void atomicSub(uint32 i, uint32 volatile* p)
  {
	__asm__ __volatile__(
						 "lock; subl %1, %0;"
						 :"=m"(*p)
						 :"ir"(i), "m"(*p)
						 :"memory");
  }

  inline void atomicSub(uint64 i, uint64 volatile* p)
  {
	__asm__ __volatile__(
						 "lock; subq %1, %0;"
						 :"=m"(*p)
						 :"ir"(i), "m"(*p)
						 :"memory");
  }

  inline void atomicAdd(uint64 i, uint64 volatile* p)
  {
	__asm__ __volatile__(
						 "lock; addq %1, %0;"
						 :"=m"(*p)
						 :"ir"(i), "m"(*p)
						 :"memory");
  }
	
  /**
   * Atomicaly exchange values value and p and return the old value that was stored in p
   */
  inline int32 atomicExchange(int32 i, int32 volatile* p)
  {
	int32 r;
	// note that lock prefix is not needed in xchg operations
	__asm__ __volatile__(
						 "xchgl %0, %1"
						 : "=r"(r), "=m"(*p)
						 : "0"(i), "m"(*p)
						 : "memory");

	return r;
  }
#ifdef ARCH64	
  /**
   * atomicExchange when ptr size is 64
   * Atomicaly exchange values value and p and return the old value that was stored in p
   */
  inline int64 atomicExchange(int64 i, int64 volatile* p)
  {
	int64 r;
	// note that lock prefix is not needed in xchg operations
	__asm__ __volatile__(
						 "xchgq %0, %1"
						 : "=r"(r), "=m"(*p)
						 : "0"(i), "m"(*p)
						 : "memory");
	return r;
  }
#endif


  /**
   * Atomicaly sets value to location using compiler barrier.
   * NOTE: We are not forcing memory barrier since even linux kernel does not do that on x86.
   */
  inline void atomicSet(int32 const value, int32 volatile* const p)
  {
	__asm__ __volatile__(
						 "movl %1, %0"
						 : "=m"(*p)
						 : "r"(value), "m"(*p)
						 : "memory");
  }

  /** Believe it! \c rep_nop() is <B>important</B> for busy-waiting
   *  loops.  It actually triggers some very real pipeline magic
   *  (whose implementation details are an internal detail, according
   *  to the chipset documents).
   *
   *  The idea is that when you recognize a 'poll' event, it can take
   *  a pipelined processor "a long time" to decide exactly which precise
   *  instruction inside its pipeline should get the poll result.  But
   *  if you're pipeline is just busy-waiting, the pipeline is full of
   *  this identical instructions and you do not really care about which
   *  precise instruction saw the polled value. So exiting the loop can
   *  be very expensive.
   *
   *  So rep nop is a special signal to the CPU that allows it to exit
   *  such loops much more quickly (by some unspecified manner).
   */
  inline __attribute__((always_inline)) void rep_nop()
  {
	asm volatile( "rep;nop": : :"memory");
  }

#define cpu_relax() rep_nop()

  //
  //  XXX THESE ARE BOGUS XXX but architecture-dependent stuff is in this file so...
  //
  //  I have no way of nicely determining what CPU/features you
  //  have... so here's a simple set that might work on x86...
  //  ... not strictly correct !!! ... I was testing on a
  //  486, so I used the most primitive one...
  //
  //  XXX For server, the efence/lfence ones are surely more correct XXX
  //  (I was just concerned with a non-xmms '486 when I was testing this)
  //
#ifdef ARCH64
#define mb() asm volatile ("lock; addq $0,0(%%rsp)":::"memory")
#define rmb() asm volatile("lock; addq $0,0(%%rsp)":::"memory")
#else
#define mb() asm volatile ("lock; addl $0,0(%%esp)":::"memory")
#define rmb() asm volatile("lock; addl $0,0(%%esp)":::"memory")
#endif
#define wmb() asm volatile ("":::"memory")
#define read_barrier_depends() do{}while(0)

#define smp_mb()	mb()
#define smp_rmb()	rmb()
#define smp_wmb()	wmb()
#define smp_read_barrier_depends()	read_barrier_depends()
#define set_mb(var, value) do { xchg(&var,value); } while(0);

  // see my Speed branch for set_64bit... I don't need it anywhere here.

} // util::

#endif // UTIL_ATOMIC_H
