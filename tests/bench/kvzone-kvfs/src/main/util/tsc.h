/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef __UTIL_TSC_H_
#define __UTIL_TSC_H_

/** \file tsc.h
 *  \brief <B>t</B>ime <B>s</B>tamp <B>c</B>ounter and APIC low-level support
 *
 *  Some low-level assembler functions to support time stamp
 *  counter routines.   These *can* be used to do nanosecond-level
 *  instruction-cycle counting if you know the proper procedure.
 *
 *  We also provide some related routines that query the local
 *  Advanced Programmable Interrupt Controller (APIC) of x86
 *  SMP systems.  Amongst its many [more important] uses, querying
 *  the APIC allows us to determine which CPU we are running on.
 *  Sampling all CPUs and determining the relation between CPU
 *  and time stamp counter readings is a basic building block for
 *  building mechanisms to monitor and use the time stamp counter
 *  effectively.  (Such advanced uses are not in tsc.h -- they will
 *  appear somehow as a high resolution submodule of boostTime.h I think).
 *
 *
 * IMPLEMENTATION NOTES (generic, related things):
 *
 *  The functions here are ones that might someday moved to
 *  include/util/, if there is any need.  More esoteric functions
 *  have been split off into tscSupport.h, which is the place
 *  to put miscellaneous helper functions related to high-res
 *  timing.
 *
 *  Note: kernel during early 2007 has a number of high-res
 *  patch ideas floating around.  Very informative to read.
 *
 *  The failsafe mode (initial version) will run on motherboards
 *  constructed in hell, and you can expect give CPU-to-CPU
 *  [independent] jitter of up to 200 microseconds.
 *
 *  Under development there is a mode that will give sub-microsecond
 *  accuracy on most motherboards/CPUs currently known.
 *
 *  Also provide some inlines for working with the 'struct timespec'
 *  return type from "clock_gettime( CLOCK_REALTIME, &result )" calls.
 *
 *  Also encapsulate support for determining APIC Id and CPU number.
 *
 *  Provide a low-level routine that measures time,tick data for
 *  all CPUs (typically 70000-100000 clock cycles to execute).
 *
 *  It is assumed that sleep and power-saving modes are not in use
 *  while the client is calling these routines, or loading up the
 *  dynamic link library.  (This is only assumed for convenience.)
 *  Note: some sleep modes on some common chips do NOT affect the
 *  time stamp counter register, because this counter register is
 *  driven "right where the clock signal enters the chip", conceptually.
 */

#include <time.h>               // struct timespec
#include <sys/sysinfo.h>        // get_nprocs() (GNU ext.)
#include <unistd.h>             // pid_t, getpid()
#include <errno.h>              // errno
#include <sched.h>              // cpu_set_t, sched_{set|get}affinity, CPU_* macros
#include <stdint.h>             // uint32_t

#include <util/gettid.h>
#include <util/exceptions.h>
#include <util/debug.h>

namespace util
{
  class Apic;
  class CpuCycler;

    
  // here are the functions we will inline below:

  /**
   * \c rdtsc() and a \c flushPipeline() are a pair of operations
   * that allow knowledgable users to measure nanosecond-scale
   * events (at least if they are locked onto one CPU, etc. etc.)
   */
  uint64_t volatile rdtsc(); // __attribute__((pure))??

  /** A single cpuid() function can be used to flush the instruction
   * pipeline, just in case anyone wants to measure nanosecond things.
   */
  void volatile flushPipeline();

  /**
   * Raw measurements for monitoring and forward-predicting
   * real time from time stamp counter ticks are useful to
   * maintain apart from manipulated data.
   *
   * \note CPU/APIC is not recorded here as raw data.
   *
   * Use \c Apic::sampleAllTscs to record a full round of
   * \c TscMeasurements in time stamp clock monitoring applications.
   */
  struct TscMeasurement
  {
	uint64_t tick;                  //*< avg of begTick and endTick
	struct timespec time;           //*< clock_gettime
	uint64_t measurementTicks;      //*< endTick - begTick
  };

  /** This executes a sysconf call, or maybe get_nprocs()?
   * for debug/test purposes.
   *
   *  The <B>uint32_t const Apic::nCpus</B> variable contains the same value,
   *  and may be faster to access directly.  (It is assumed that the number
   *  of processors does not vary).
   */
  uint32_t const getNCpus();


  /** Wrap things which are applied to each CPU or all CPUs.
   *
   * \note:
   *  Named 'Apic' only because the CPU to which each Apic is
   *  associated has a time stamp counter, not because we supply
   *  an interface for actually handling interrupts.
   */
  class Apic
  {
  public:
	/** I used to call this one 'util::getApicId()' */
	static uint32_t id();

	/**
	 *  Translate \c id() result into a cpu number.
	 *  This is the same cpu number that you would use,
	 *  for example, in 'util::setAffinity( int32 cpu )'
	 *  calls (see misc.h).
	 *
	 *  Only reliable <E>after</E> somebody has called 'Apic::init()'
	 *
	 *  (takes explicit argument to force awareness of optimization
	 *   opportunities.   For example, comparing an initial \c id() with
	 *   a final \c id() saves a needless translation.)
	 */
	static uint32_t cpu( uint32_t apicId ); // __attribute__((pure));

	/** I will ASSUME that the number of CPUs is constant and
	 * does not change.  This just looks up a prestored value,
	 * avoiding the function call of \c util::getNCpus() .
	 */
	static uint32_t const getNCpus();

	/** Initialize apic to cpu translation.
	 *
	 *  Only the first call will initialize our private variables.
	 *  Subsequent calls will [quickly] be ignored, since we
	 *  <E>assume</E> that processors will neither appear nor disappear.
	 *
	 *  getCpu() will only return trustworthy results after
	 *  initialization.
	 *
	 *  (This would typically be run during the startup routine
	 *  of the high resolution sub-module of boostTime.h)
	 */
	static void init(); // noninline, slow the first time it's called

	/** Do a round of tick and time sampling across all CPUs.
	 *
	 * Retries a limited number of times if things take more cycles
	 * than expected (this is our \c measurementTicks accuracy).
	 * This might indicate that we were interrupted at an
	 * unfortunate moment.
	 *
	 * TscMeasuremente MUST point to valid memory regions that can
	 * hold an array of \c Apic::nCpus measurements.
	 *
	 * This is a useful base operation in monitoring and locking
	 * time stamp counters to system time amongst various CPUs.
	 *  
	 * Upon return we'll be running on same CPU,
	 * but with setAffinity(-1)
	 *  (XXX TBD: save/restore our particular locked-to-CPU state)
	 */
	static void sampleAllTscs( TscMeasurement *rawData );

	/** accessor, only publicized for unittests. This is a lowish
	 * estimate for how many ticks clock_gettime() might take. */
	static uint64_t getTicks4Systime();

  private:
	/** current x86 chips are limited to 256 apicids.
	 * 
	 *  I use the APIC id value '\xFF' to indicate an unintialized initial
	 *  state of Apic data.  In the unlikely event that your motherboard
	 *  supports hundred of processors, I suppose it would be possible that
	 *  this id is actually used.  'init()' will fail if it detects this condition.
	 *  In that case, rewrite this to use a separate bool to flag an
	 *  uninitialized state.
	 */
	static uchar apic2cpu[256];

	/** A low typical value for how many cycles calling clock_gettime()
	 *  can be expected to take.  When measuring Tick,Time pairs,
	 *  we do internal checks and retry measurements if the time to
	 *  do the measurement is too long (probably we had bad luck
	 *  and lost our time slice).
	 *
	 *  We will typically try for valid rdtsc,clock_gettime,rdtsc
	 *  measurements that take under double the typical value
	 *  (ticks4Systime is likely going to be around 8000 clock cycles)
	 */ 
	static uint64_t const ticks4Systime;

	/** call the function just once, just look up this value and
	 * assume number of processors does not change. */
	static uint32_t const nCpus;

  }; // end class util::Apic

  /**
   * Specialized (override) versions of the misc.h setAffinity() call.
   * NOTE: It is highly recommended to use class \c CpuCycler if
   * your goal is to cycle a thread through all CPUs.
   *
   * for tsc and hrFastTime, we wish to leave any thread affinity
   * unchanged, which is not the simplistic behaviour provided by
   * the misc.cpp \c util::setAffinity(int32 cpuid) call.
   *
   * This function stores various useful variables so that a routine
   * that uses forceAffinity can be restored to executing on the
   * same CPU, with the same CPU affinity.
   *
   * Such routines could follow something like the following usage pattern:
   * \verbatim
   * //#include ".../tsc.h"
   * pid_t pid;
   * uint32_t apicOld, cpuOld;
   * cpu_set_t affinityOld;
   * getAffinity( pid, apicOld, cpuOld, affinityOld );
   *
   * CPU_ZERO(affinityNew);
   * CPU_SET(0, affinityNew);
   * forceAffinity( pid, affinityNew )
   * yieldIfNotOnCpu( 0 );
   * ... code that MUST execute on cpu 0 ...
   *
   * CPU_CLR(0, affinityNew);
   * CPU_SET(1, affinityNew);
   * forceAffinity( pid, affinityNew )
   * yieldIfNotOnCpu( 1 );
   * ... code that MUST execute on cpu 1 ...
   *
   *
   * CPU_CLR(1, affinityNew );
   * CPU_SET( cpuOld, affinityNew );
   * forceAffinity( cpuOld, affinityNew );    // first force cpuOld
   * yieldIfNotOnApic( apicOld ); // (marginally quicker to test apicId)
   * forceAffinity( pid, affinityOld )        // THEN expand cpu permissions
   *
   * ... now we are back with same CPU affinity and running on the
   *     old CPU again ...
   * \endverbatim
   *
   * Another usage pattern is to temporarily prohibit migration to other cpus:
   * \verbatim
   * pid_t pid;
   * uint32_t aid, cpu;
   * cpu_set_t affinity0;
   * cpu_set_t affinity;
   * CPU_ZERO( &affinity );
   * // restrict to a current CPU (temporarily)
   * util::getAffinity( pid, aid, cpu, affinity0 );
   * CPU_SET( cpu, &affinity );
   * util::forceAffinity( pid, affinity );
   *
   * {
   *     ... do stuff without possibity of cpu migration ...
   * }
   *
   * // we are still on original CPU, relax cpu permissions again.
   * util::forceAffinity( pid, affinity0 );
   * \endverbatim
   *
   * These affinity-related functions should NOT be moved into
   * include/util/, please. Split them into a private utilkit
   * header first, please.
   */
  void getAffinity( pid_t &pid, uint32_t &apicId, uint32_t &cpuId, cpu_set_t &affinity );

  /**
   * Unlike setAffinity(int32 cpuid), this call MUST unconditionally
   * execute a \c sched_setaffinity.
   *
   * client should use CPU_ZERO, CPU_SET and CPU_CLR macros to
   * explicitly manipulate affinity.
   */
  void forceAffinity( pid_t pid, cpu_set_t &affinity );

  /** convenience routine */
  void yieldIfNotOnApic( uint32_t apicId );
  /** convenience routine */
  void yieldIfNotOnCpu( uint32_t cpuId );

  /** A convenient, simple, restrictive way to cycle the current thread
   * through all available CPUs.  We guarantee that after destruction
   * we will be back on original CPU with the same affinity.
   *
   * CpuCycler is non-reentrant.
   *
   * Use of CpuCycler before Apic::init() is called yields undefined behaviour.
   * (Probably a setAffinity error if you're lucky)
   *
   * Simple => only 2 suggested ways to use a CpuCycler.
   *
   * <B> #1 scoped 'lock to current CPU' </B>. Just run locked down to
   * current CPU, and guarantee restore of original CPU permissions after
   * destruction:
   * \verbatim
   * {
   *     CpuCycler cc;
   *     cout<<"This thread is now on CPU #"<<cc.cpu()<<endl;
   *     ; // other less important work here
   * }
   * \endverbatim
   * Here CPU is never changed, but once CpuCycler is contructed
   * we guarantee we won't switch CPUs.  And after CpuCycler is
   * destroyed we guarantee that the original CPU permissions
   * are restored.  (Slightly different from existing
   * util::setAffinity call in misc.h).
   * 
   * <B> #2 scoped 'cycle through all CPUs once'. </B>
   * \verbatim
   * {
   *     CpuCycler cc;
   *     do
   *     {
   *        cout<<"This thread is now on CPU #"<<cc.cpu()<<endl;
   *        ; // other less important work here
   *      }
   *      while( cc.nextCpu() );
   *      // at this point we happen to be back on original CPU
   * }
   * // at this point we are also back with same CPU affinity settings
   * \endverbatim
   *
   * If you have 4 cpus and you start on CPU 2, then the output
   * would be
   * \verbatim
   * This thread is now  on CPU # 2
   * This thread is now  on CPU # 3
   * This thread is now  on CPU # 0
   * This thread is now  on CPU # 1
   * \endverbatim
   * 
   * Any non-conformant usage provides NO GUARANTEE of correctness,
   * although debug mode asserts that things look OK.  For fancier
   * needs, you may use the getAffinity(), forceAffinity(), yield*()
   * directly.
   */
  class CpuCycler
  {
  public:
	/** Record current affinity and CPU settings, initialize for cycling
	 *  this thread across all cpus.  Return with thread locked down onto
	 *  original \c this->cpu();
	 */
	CpuCycler();

	/** Return true if next cpu is different from original CPU
	 *  When we return, the thread is locked down onto \c this->cpu()
	 */
	bool nextCpu();

	/** What CPU are we running on?  Between constructor and destructor
	 * and nextCpu() calls, we GUARANTEE that this thread is locked
	 * down onto this->getCpu().
	 */
	uint32_t getCpu() const;

	/** Assume that we're back to original CPU and expand back to
	 *  original CPU permissions.
	 *
	 *  Behaviour is undefined if getCpu() is not the original one.
	 *  This never happens if you follow the intended iteration pattern.
	 *
	 *  You want to have an opportunity to `break' out of the suggested
	 *  usage pattern?  Write a new class for that.
	 */
	~CpuCycler();

  private:
	void lockdown();

  private:
	// logically const initial [and final] state
	pid_t pid;
	uint32_t aid0;
	uint32_t cpu0;
	cpu_set_t affinity0;

	// iteration variables
	uint32_t cpu;
	cpu_set_t affinity;
  };







  ///////////////////////  Below: inlines .....................................
    
  // You will often see the following (i.e. in kernel code or "high-res"
  // timers before and rdtsc assembler instruction:
  //#if defined(CPU_HAS_NONSERIALIZING_RDTSC)
  //                // PII and PPRO (these support instruction reordering)
  //                // (not required for Pentium or Pentium with MMX)
  //                "cpuid\n\t"
  //#endif
  //
  // instructional note:  The purpose of the cpuid instruction is to flush
  // the processor pipeline.  This is required if you want to measure events
  // that take only a few cycles (and you've locked down to one cpu, and you
  // take some precautions to avoid your measurement routines being affected
  // by yielding, etc. etc.)
  //
  // On server44, this flush could take 200-500 cycles (very roughly).  This
  // is a lot.  And we are particularly interested in fast reads, and our
  // target resolution is "sub-millisecond" (i.e. microsecond or worse
  // time resolution).
  //
  // So... getting a "now" accurate to 1 clock cycle (at the expense of
  // a pipeline full of cycles to get that measurement)
  // is NOT important for our purposes.
  //
  // Therefore, util::rdtsc() gives you "whatever" time stamp counter
  // value the processor chooses (since it is allowed to reorder
  // any instructions within it's pipeline "at will").
  //
  // i.e. for analyzing propagation of timing errors, you would be safe
  // to assume a time measurement error of +0 to -500 ns from util::rdtsc()
  // (with standard error analysis mindset that error estimates don't need
  // much precision... 1 significant digit (max) is typically forced upon
  // students just to make them aware of this.)
  //
  inline __attribute__((always_inline)) volatile uint64_t rdtsc()
  {
#if defined (__GNUC__)
#ifndef ARCH64
	uint64_t now;
	asm volatile (
				  "rdtsc"
				  : "=A" (now)
				  :
				  : "memory"
				  );
	return now;
#else
	uint32_t lo, hi;
	asm volatile ( 
				  "rdtsc"
				  : "=a" (lo), "=d" (hi)
				  :
				  : "memory"
				   );
	return (uint64_t)hi << 32 | lo;
#endif
#else
#error "unsupported compiler"
#endif
  }

  inline __attribute__((always_inline)) uint32_t const getNCpus()
  {
	//sysconf(_SC_NPROCESSORS_CONF) - OK, let's assume they're all available.
	// GNU extension: sys
	return get_nprocs();
  }

  inline __attribute__((always_inline)) uint32_t const Apic::getNCpus()
  {
	return Apic::nCpus;
  }

  /** cpuid opcode 1 returns the unique APIC ID in ebx[31:24] */
#define INITIAL_APIC_ID_BITS 0xFF000000U
#ifdef ARCH64
  inline __attribute__((always_inline)) uint32_t Apic::id()
  {
	//return ((util::cpuid_ebx(1U) & INITIAL_APIC_ID_BITS) >>24);
	//  Hard-coded to avoid exposing a more complete set of cpuid_XXX
	//  assembler code.
	uint32 ebx;
	__asm__ volatile (
					  "pushq %%rbx\n\t"
					  "cpuid\n\t"
					  "movl %%ebx,%1\n\t"
					  "popq %%rbx"
					  : "=a" (ebx)
					  : "0" (1)
					  : "cx", "dx" );
	return (ebx & INITIAL_APIC_ID_BITS) >>24U;
  }
#else
  inline __attribute__((always_inline)) uint32_t Apic::id()
  {
	//return ((util::cpuid_ebx(1U) & INITIAL_APIC_ID_BITS) >>24);
	//  Hard-coded to avoid exposing a more complete set of cpuid_XXX
	//  assembler code.
	uint32 ebx;
	__asm__ volatile (
					  "pushl %%ebx\n\t"
					  "cpuid\n\t"
					  "movl %%ebx,%1\n\t"
					  "popl %%ebx"
					  : "=a" (ebx)
					  : "0" (1)
					  : "cx", "dx" );
	return (ebx & INITIAL_APIC_ID_BITS) >>24U;
  }

#endif
#undef INITIAL_APIC_ID_BITS

  inline __attribute__((always_inline)) uint32_t Apic::cpu( uint32_t apicId )
  {
	return Apic::apic2cpu[ apicId ];
  }

#ifdef ARCH64
  inline __attribute__((always_inline)) void volatile flushPipeline()
  {
	// push/pop of rbx just to support -fPIC compilation
	__asm__ volatile (
					  "pushq %%rbx\n\t"
					  "cpuid\n\t"
					  "popq %%rbx\n\t"
					  :
					  : "a" (1)
					  : "cx", "dx"
					  );
  }
#else
  inline __attribute__((always_inline)) void volatile flushPipeline()
  {
	// push/pop of ebx just to support -fPIC compilation
	__asm__ volatile (
					  "pushl %%ebx\n\t"
					  "cpuid\n\t"
					  "popl %%ebx\n\t"
					  :
					  : "a" (1)
					  : "cx", "dx"
					  );
  }
#endif

  inline __attribute__((always_inline,nothrow)) uint64_t Apic::getTicks4Systime()
  {
	return Apic::ticks4Systime;
  }

  inline __attribute__((always_inline)) void yieldIfNotOnApic( uint32_t apicId )
  {
	if( Apic::id() != apicId )
	  {
		//_LOG(INFO,"Surprise! yieldIfNotOnApic was actually useful on your system!");
		sched_yield(); // just in case (usually doesn't run)
	  }
  }

  inline __attribute__((always_inline)) void yieldIfNotOnCpu( uint32_t cpuId )
  {
	if( Apic::cpu( Apic::id() ) != cpuId )
	  {
		//_LOG(INFO,"Surprise! yieldIfNotOnCpu was actually useful on your system!");
		sched_yield(); // just in case (usually doesn't run)
	  }
  }

  inline void getAffinity( pid_t &pid, uint32_t &apicId, uint32_t &cpuId,
						   cpu_set_t &affinity )
  {
	apicId = Apic::id();
	cpuId = Apic::cpu( apicId );
	pid = getpid();
	CPU_ZERO( &affinity ); // Is this required? (not explicitly documented as a requirement)
	if( sched_getaffinity( pid, sizeof(affinity), &affinity ) < 0 )
	  {

		_LOG(INFO, "sched_getAffinity failed: errno "<<errno);
		//_THROW_ERR(SystemException, "sched_getAffinity failed: errno "<<errno);
	  }
  }

  inline void forceAffinity( pid_t __attribute__((unused)) pid, cpu_set_t &affinity )
  {
	if( sched_setaffinity( gettid(), sizeof(affinity), &affinity ) < 0 )
	  {
		// errno=22 can be a result of trying to use a CpuCycler without having
		// called util::Apic::init().  util::Apic::init() will be called automatically
		// when initializing util::FastTime.  Otherwise, you will need to explicitly
		// call it.
		_LOG(INFO, "sched_setaffinity(pid,affinity) failed: errno "<<errno);
		//_THROW_ERR(SystemException, "sched_setaffinity(pid,affinity) failed: errno "<<errno);
	  }
  }

  inline uint32_t CpuCycler::getCpu() const
  {
	return this->cpu;
  }
  inline void CpuCycler::lockdown()
  {
	CPU_SET( this->cpu, &this->affinity );
	//_LOG(INFO,"CpuCycler::lockdown to cpu ",_VAR(this->cpu));
	forceAffinity( this->pid, this->affinity );
	yieldIfNotOnCpu( this->cpu );
  }

  inline CpuCycler::CpuCycler()
  {
	//_DASSERT( Apic::cpu(0) == ((unsigned char)(0xFF)),
	//            "You must util::Apic::init() before using a CpuCycler" );
	CPU_ZERO( &this->affinity0 );
	util::getAffinity( this->pid, this->aid0, this->cpu0, this->affinity0 );
	CPU_ZERO( &this->affinity );
	this->cpu = this->cpu0;
	this->lockdown();
  }

  inline bool CpuCycler::nextCpu()
  {
	CPU_CLR( this->cpu, &this->affinity );
	if( ++this->cpu >= Apic::getNCpus() )
	  {
		this->cpu = 0U;
	  }
	this->lockdown();
	return this->cpu != this->cpu0;
  }

  inline CpuCycler::~CpuCycler()
  {
	// *after* merge, investigate switch to _PASSERT XXX
	//endingOnOriginalCpu,
	//LX( const bool endingOnOriginalCpu = (this->cpu == this->cpu0) ),
	//CT( endingOnOriginalCpu ),
	_DASSERT( this->cpu == this->cpu0, "Improper CpuCycler usage\n");
	// nextCpu() already ensures we are locked down on original CPU when done
	// so all we need to do is expand back to original cpu permissions
	forceAffinity( this->pid, this->affinity0 );
  }
} // util::

#endif // __UTIL_TSC_H_
