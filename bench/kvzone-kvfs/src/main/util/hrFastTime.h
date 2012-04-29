/*
 * Copyright (c) 2007 NEC Laboratories, Inc. All rights reserved.
 */

/** \file hrFastTime.h
 *  \brief API for a 'high-resolution' sub-module of boostTime.h
 *
 *  If/when the linux kernel provides those v*() functions for direct access
 *  to kernel timer data from userspace, this entire functionality should be
 *  replaced by that mechanism, if possible.
 */
#ifndef HRFASTTIME_H
#define HRFASTTIME_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <util/boostTime.h> // epoch static variable
#include "tsc.h"  // (sigh) for struct TscMeasurement
#include "seqlock.h"

#include <vector>

// if rates agree to one part in a million over time spans bigger than
// four seconds, we will assume common CPU clocks.  O/W at some point
// we probably had a system time jump.
//  Note, usually this is under 1.e-6 but the threshold REALLY depends
//  on how long you measure ticks for too...
// Modify this to force one model or the other for testing...
//
#define HRFASTTIME_MAXRELDEVN_TARGET 1.e-13 /*never, always assume most general model*/

namespace util
{
  class TscExtrapolator; // opaque time extrapolation objects.
  class TextrapThread;   // opaque updater for time extrapolation objects.

  /** Note: the easiest way to guarantee monotonicity is to lock to
   * a single CPU.  If you don't care about system time jumps of more
   * than a half a second within a 6 second period, you can still use
   * progSec(), which is guaranteed monotonic on each CPU, but might have
   * some cumulative offset of all too-big time jumps.
   *
   * progSec() not locked to a CPU might be non-monotonic for very short 
   * durations because CPU timers are totally independent of each other.
   * They each lock to clock_gettime independently, and have random
   * (small) error with respect to one another.
   *
   * Each CPU is typically locked to within about 33 +/- 150 microseconds
   * (st.dev.) of the system clock_gettime value.
   *
   * A suitable low period of 6 seconds between clock rates seems
   * reasonable (dictated by temperature drift of the motherboard
   * quartz crystal, which can vary the clock rate easily up to 200 kHz
   * over a 40 minute period, without even trying to heat things up).
   *
   * NOTE: start and stop could easily be hooked up with the
   * util::FastTime thing to avoid all HrFastTime threads.
   * All we need is to setAffinity and call the 'measure()'
   * routine of one of the CPUs every 1.5 seconds.
   * (However we want this to happen based on SIG counting,
   * and not the util::FastTime counter, which might be
   * stopped ?)
   */
  class HrFastTime
  {
	//  should eventually be private or cleaned up.
  public:
	static struct timespec systime0; //*< time we started
	static double dsystime0;         //*< time we started (seconds)

  public:
	/** Start the time stamp counter time system.  This provides per
	 * CPU locks to system time by querying on each CPU every 6 seconds
	 * or so.  Over 100 minutes, I measured average errors of
	 * 33 +/- 150 microseconds.  The precision is microseconds.
	 * The time stamp counters can be read by multiple users
	 * concurrently.
	 *
	 * This default interface is for testing WITHOUT FastTime.
	 *
	 * NEW: \c startExtrapolationThread() can be used to tell
	 * this guy NOT to start threads.
	 * 
	 * needThread == false  means "I promise to be faithful
	 * to you" and provide regular updateExtrapolation()
	 * calls roughly every getPeriod() seconds.
	 */

	/** ----------- NEW -------------- non-thread interface
	 *
	 * so I can run as sub-module of some other thread that
	 * will provide my updates very regularly.
	 */
	static void startExtrapolation(
								   uint32_t sleepTime/*seconds*/ = 6U,
								   bool needThread = false
								   );
	/** running as submodule I don't provide a thread, but a
	 * public function the really must be updated every
	 * sleepTime (6 seconds) (approximately - I use sleep
	 * for it, so I'm obviously not terribly concerned with
	 * a highly accurate sleepTime.)
	 */
	static void updateExtrapolation();

	//
	/** ... and a version for running it as a submodule of FastTime.
	 *
	 * .... this will soon be the only way to run it.
	 */
	static void stopExtrapolation();


	/** Here are the low-level functions (not from boostTime).
	 *  These are as fast as possible.  The current extrapolator
	 *  core locks to systime very well, and provides a cumulative
	 *  time jump value.
	 *
	 *  These functions interrogate which CPU you are using,
	 *  and then retrieve a time extrapolation appropriate
	 *  for the time stamp counter on that CPU.
	 *
	 *  NOTE: they are slower than util::FastTime because they
	 *  execute a slow variant of the 'cpuid' assembler instruction,
	 *  twice. You can estimate 1500 cycles (I often see around
	 *  1000 cycles).  This is still faster than the roughly
	 *  8000-cycle clock_gettime(..).
	 *
	 * \note:
	 *  Faster version is possible if all CPUs are synched,
	 *  AND time stamp counters are synchronized, but the 
	 *  HrFastTime monitor functions that recognize this
	 *  desirable (and frequent) condition have not been written.
	 *  (First goal is to recognize common clock rates and
	 *   provide an extrapolator mode in which any errors
	 *   are almost uniform amongst CPUs so that relative
	 *   time values from different threads on different
	 *   CPUs are still accurate).
	 *  (Then we can think about the next one, but it is low priority)
	 */
	static long double getSysSec();
	/**
	 *  The cumulative time jump value is used to correct the
	 *  systime approximation (which is allowed to jump backward
	 *  for jumps bigger than around 0.5 seconds) into something
	 *  that provides a non-decreasing "time since program start".
	 *
	 *  ( I do not yet provide a long-term slew of program time
	 *    to system time, so the return value for now should really
	 *    agree well with some "wall clock time" )
	 */
	static long double getProgSec();
            
	/**
	 *  If strictly increasing is important, I suggest to
	 *  wrap time functions and if non-increasing to add some
	 *  machine epsilon to the last returned value --- use a
	 *  seqlock to provide the guaranteed-increasing functionality
	 *  to all threads.
	 *
	 *  getIncreasingXXX() TBD XXX
	 */

	/** We just use sleep.  The update period is not critical. */
	static uint32_t getPeriod(); /* in seconds */

	// TODO: this will change to use the tsc.h version....
	/** 3x faster (avoid std::vector).  ticks, times are arrays[ncpus]
	 * approx. on par with handwritten version. (meas. 70-100 kcycles, ~ 50 us)
	 * static void sampleAllCpus( uint64_t *ticks, struct timespec *times, uint32_t ncpus );
	 */

  private:
	/** This is the only "write" operation to the high-res extrapolation data.
	 * We use a write lock here, and readers (multiple simultaneous readers allowed)
	 * will retry their operations in case of conflicts with this code.
	 */
	void doUpdateExtrapolation();

  private:
	/** opaque internal class */
	friend class TextrapThread;
	/** opaque internal class */
	friend class TscExtrapolator; // access common sleepTime value

	// internal helpers....
	// the essential core....
	static util::TextrapThread *thread;    // one of these
	static util::TscExtrapolator *textrap; // array of nCpus of them
	static util::TscMeasurement *tmeasure; // array of nCpus of them
	static uint32_t sleepTime;

	// other stuff....
	static bool initialized; //*< true between start/stop calls, for non-threaded use
	static bool assumeCommonCPUclocks; // always false for now (pessimistic case)


	// NEW: client threads will infrequently invoke doUpdateExtrapolation();
	// start/stop/update ops take write locks on the extrapolator data now.
	static SeqLock lockHrFastTimeUpdate;
	static uint32_t volatile isHrFastTimeUpdateRunning;
  };

#ifdef HRFT_MAIN
#define HRFT_INLINE
#else
#define HRFT_INLINE inline
#endif

  HRFT_INLINE uint32_t HrFastTime::getPeriod() /* in seconds */
  {
	return HrFastTime::sleepTime;
  }
#undef HRFT_INLINE

} // util::
#endif // HRFASTTIME_H

