/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 *
 * Implementation of boost_date_time helper functions/classes.
 */

#include <iostream>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <signal.h>
#include <sys/time.h>
#include <boost/scoped_ptr.hpp>

#include <util/string.h>
#include <util/atomic.h>
#include <util/exceptions.h>
#include <util/boostTime.h>

#include "hrFastTime.h"         // experimental sub-module


using namespace std;
using boost::gregorian::date;

namespace util 
{
	//============================================================================

	timespec timeDuration2TimeSpec(time_duration const& duration)
	{
		timespec ts;

		ts.tv_sec = timeDurationInSeconds(duration);
		ts.tv_nsec = timeDurationNanosecs(duration);

		return ts;
	}

    timeval timeDuration2TimeVal(time_duration const& duration)
	{
		timeval tv;

		tv.tv_sec = timeDurationInSeconds(duration);
		tv.tv_usec = timeDurationMicrosecs(duration);

		return tv;
	}

   	xtime timeDuration2XTime(time_duration const& duration)
	{
		xtime xt;

		xt.sec = timeDurationInSeconds(duration);
		xt.nsec = timeDurationNanosecs(duration);

		return xt;
	}

	xtime ptime2xtime(ptime const& pt)
	{
		tm t;
 		
		time_duration timeOffset = pt.time_of_day();
		t.tm_sec = timeOffset.seconds();
		t.tm_min = timeOffset.minutes();
		t.tm_hour = timeOffset.hours();
		
		date theDate = pt.date();
		t.tm_mday = theDate.day();
		t.tm_mon = theDate.month();
		t.tm_year = theDate.year();

		xtime xt;
		 
		xt.sec = mktime(&t);
		xt.nsec = timeDurationNanosecs(timeOffset);
		
		return xt;
	}
	
	uint64 timeDurationInSeconds(time_duration const& duration)
	{
		return timeDurationInFractionalSeconds<1>(duration);
	}

	uint64 timeDurationInMillisecs(time_duration const& duration)
	{
		return timeDurationInFractionalSeconds<1000>(duration);
	}

	uint64 timeDurationInMicrosecs(time_duration const& duration)
	{
		return timeDurationInFractionalSeconds<1000000>(duration);
	}

	uint64 timeDurationInNanosecs(time_duration const& duration)
	{
		return timeDurationInFractionalSeconds<1000000000>(duration);
	}

	uint32 timeDurationMillisecs(time_duration const& duration)
	{
		return timeDurationFractionalSeconds<1000>(duration);
	}

	uint32 timeDurationMicrosecs(time_duration const& duration)
	{
		return timeDurationFractionalSeconds<1000000>(duration);
	}

	uint32 timeDurationNanosecs(time_duration const& duration)
	{
		return timeDurationFractionalSeconds<1000000000>(duration);
	}

	//============================================================================
	//                         FastTime implementation
	//============================================================================

	void FastTime::startFastTimer( bool highres /*=true*/ )
	{
		FastTime::initialize( highres );
	}

	void FastTime::startFastTimer(void (*timeOutOfSynchArg) (time_duration const &)
                , bool highres /*=true*/ )
	{
		timeOutOfSynch = timeOutOfSynchArg;
		FastTime::initialize( highres );
	}

	void FastTime::finishFastTimer()
	{
		FastTime::destroy();
		timeOutOfSynch = NULL;
	}

	// variable protecting currentTime
	static volatile int32 nestedHandler;

        // variable representing HrFastTime period
        static uint32 usingHighRes;
        static uint32 hrMaxSleepTimeCount;
        static uint32 hrSleepTimeCount;
        static volatile uint32_t isHrFastTimeUpdateRunning;

        void FastTime::initialize( bool highres )
        {
            PASSERT(currentOffset == NULL, "Fast timer already started");
            LOG(INFO, "FastTime will be incremented by threads " << frequency << " times per second");
            usingHighRes = highres;
            util::Apic::init(); // just in case (paranoia)
            DLOG(1, "Apic::init() was called");
            if( usingHighRes )
            {
                // Begin the high-resolution sub-module
                HrFastTime::startExtrapolation(); // normally takes between approx 1 and 3 seconds
                // I need to nudge HrFastTime every so often ...
                hrSleepTimeCount    = 0U;
                hrMaxSleepTimeCount = HrFastTime::getPeriod()/*seconds*/ * FastTime::frequency;
                LOG(INFO, "hrMaxSleepTimeCount is fixed at "<<hrMaxSleepTimeCount<<", current count is "<<hrSleepTimeCount);
                FastTime::isHrFastTimeUpdatePending = false;
                //    NEW: values are set up *before* the setitimer call.
            }

            ptime now = util::getTime();
            time_duration diff = now - ptime(epoch, time_duration());
            // from now on getFastTime() will start working
            offset1 = diff.total_milliseconds() / (1000 / frequency);
            currentOffset = &offset1;
            { // first set handler
                struct sigaction oldSa;
                struct sigaction sa;
                sa.sa_handler = incrementTime;
                sa.sa_flags = SA_RESTART | SA_NOMASK;
                sigemptyset(&sa.sa_mask);

                if (sigaction(SIGALRM, &sa, &oldSa) != 0)
                {
                    THROW_ERR(RuntimeException, "Unable to execute sigaction for SIGALRM");
                }
                intptr_t handlerInt = reinterpret_cast<intptr_t>(oldSa.sa_handler);

                if (handlerInt != reinterpret_cast<intptr_t>(SIG_DFL) && handlerInt != reinterpret_cast<intptr_t>(SIG_IGN))
                {
                    LOG(INFO, "is equal DFL: " << (handlerInt == 1));
                    LOG(INFO, "is equal IGN: " << (handlerInt == 1));
                    FAILURE("Someone already registered handler on SIGALRM - different from incrementTime: " << hex << handlerInt << ", SIG_DFL = " << SIG_DFL << ", SIG_IGN = " << SIG_IGN);
                }
                if (oldSa.sa_flags & SA_SIGINFO)
                {
                    FAILURE("Someone already registered sigaction handler on SIGALRM: " << oldSa.sa_sigaction);
                }
            }

            { // then set itimer
                struct itimerval oldTimer;
                struct itimerval timer;
                timer.it_interval.tv_sec = 0;
                // subtract signal handling time - OS sends another signal once handler is finished
                // incrementTime() function synchronizes fast time with OS time few times in a second
                // to avoid desynchronization
                timer.it_interval.tv_usec = 1000000 / frequency - 1000;
                timer.it_value = timer.it_interval;

                if (setitimer(ITIMER_REAL, &timer, &oldTimer) != 0)
                {
                    THROW_ERR(RuntimeException, "Unable to execute setitimer() with tv_usec = " << timer.it_interval.tv_usec);
                }

                if ((oldTimer.it_value.tv_sec != 0 || oldTimer.it_value.tv_usec != 0) &&
                    (oldTimer.it_value.tv_sec != timer.it_value.tv_sec || oldTimer.it_value.tv_usec != timer.it_value.tv_sec))
                {
                    FAILURE("Unable to set itimer() - previous timer has nonzero it_value set to different frequency. Old it_value = " << oldTimer.it_value.tv_sec << " " << oldTimer.it_value.tv_usec << ", new it_value = " << timer.it_value.tv_sec << " " << timer.it_value.tv_usec);
                }
            }

            DLOG(1, "Scheduled timer with diff = " << offset1);
        }

	void FastTime::destroy()
	{
		PASSERT(currentOffset != NULL, "Calling finish twice?");
		LOG(INFO, "Finishing fast time incrementation");

		// from now on getFastTime() will start working
		struct itimerval timer;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		timer.it_value = timer.it_interval;

		if (setitimer(ITIMER_REAL, &timer, NULL) != 0)
		{
			THROW_ERR(RuntimeException, "Unable to execute setitimer() with tv_usec = " << timer.it_interval.tv_usec);
		}
		
		struct sigaction sa;
		sa.sa_handler = SIG_DFL;
		// allow nested signal handlers (otherwise remaining threads receive the signal and execute incrementTime())
		sa.sa_flags   = 0;
		sigemptyset(&sa.sa_mask);

		if (sigaction(SIGALRM, &sa, NULL) != 0)
		{
			THROW_ERR(RuntimeException, "Unable to execute sigaction for SIGALRM");
		}

		// make sure all pending signal handlers finish
		while (util::atomicAddReturn(1, &nestedHandler) != 1)
		{
			util::atomicAddReturn(-1, &nestedHandler);
			sched_yield();
		}
		// now we can disable fastTimer safely
		currentOffset = NULL;
		util::atomicAddReturn(-1, &nestedHandler);

                if( usingHighRes )
                {
                    HrFastTime::stopExtrapolation();
                    hrSleepTimeCount    = 0U;
                    FastTime::isHrFastTimeUpdatePending = false;
                }
	}

	time_duration FastTime::getFrequency()
	{
		return boost::posix_time::milliseconds(1000 / frequency);
	}

	// NOTE: do not log anything in this function - it may be called from inside logger
	void FastTime::incrementTime(int32)
	{
		// this operation needs to be atomic since we cannot be sure that only one thread executes this handler
		// it is possible that while one thread is executing signal handler another signal comes and different
		// thread starts its handler simultanously
		if (util::atomicAddReturn(1, &nestedHandler) == 1)
		{
			// do not allow nested handlers to execute
			uint32 numInc = 1;
			// check clock every 31 times for synchronization
			if ((*currentOffset & 0x1f) == 0x1f)
			{
				uint32 const intervalMilliseconds = 1000 / frequency;
				boost::posix_time::time_duration const interval = boost::posix_time::milliseconds(intervalMilliseconds);
				ptime const fastTime = FastTime::getTime();
				ptime const current = util::getTime();

				time_duration const diff = (current - fastTime) - interval;

				if (timeOutOfSynch && diff != time_duration())
				{
					// call out of synch function - we are not exactly on schedule
					(*timeOutOfSynch)(diff);
				}

				if (diff.is_negative())
				{
					numInc = 0;
				}
				else
				{
					if (diff >= interval)
					{
						// adjust drift even further
						numInc += (diff.total_milliseconds() / intervalMilliseconds);
					}
				}
			}

                        // This version has hrSleepTimeCount as ticks since the last reset.
                        // This will now be ~6 seconds plus the delay between flagging
                        // `update-needed' and actually doing the update.
                        // (Alt. versions also possible and also reasonable)
                        if( usingHighRes && !isHrFastTimeUpdatePending )
                        {
                            // logging within signal handler is illegal (can crash or worse):
                            //LOG(INFO,"hrSleepTimeCount="<<hrSleepTimeCount<<" numInc="<<numInc);
                            if ( (hrSleepTimeCount+=numInc) > hrMaxSleepTimeCount )
                            {
                                isHrFastTimeUpdatePending = true; // flag for external friend
                                hrSleepTimeCount = 0U;            // internal counter
                            }
                        }



			// following line is equivalent to stating
			// static_cast<uint32>(*currentOffset) + numInc <= 0xffffffff
			if (static_cast<uint32>(*currentOffset) <= (~ numInc))
			{
				// this operation is safe for multithreading - only one thread is modifying this value
				(*currentOffset) += numInc;
			}
			else if (numInc != 0)
			{
				// we are about to increment higher int32 - so need to switch to alternate uint64
				if (currentOffset == &offset1)
				{
					offset2 = offset1;
					// first increment
					offset2 += numInc;
					// then change pointer
					currentOffset = &offset2;
				}
				else
				{
					offset1 = offset2;
					// first increment
					offset1 += numInc;
					// then change pointer
					currentOffset = &offset1;
				}
			}
		}
		util::atomicAddReturn(-1, &nestedHandler);
                
	}

	uint64 volatile * volatile FastTime::currentOffset;
	uint64 volatile FastTime::offset1;
	uint64 volatile FastTime::offset2;
	void (* FastTime::timeOutOfSynch) (time_duration const & );
        bool volatile FastTime::isHrFastTimeUpdatePending;

	//============================================================================

        // This particular one is basically a glorified fast approximation to
        // clock_gettime.  It's just the most convenient way to implement this
        // function with least new code.  It will only observe clock jumps of
        // up to half a second without complaining.
        //
        // For averaging it WILL allow backwards time.  (If you want a
        // forward moving time that ignores big time jumps, you can call
        // HrFastTime::getProgSec().   That value is strictly increasing
        // on each CPU, but there is still +/- jitter between CPUs).
        // HrFastTime::getProgSec() will always give reasonable
        // time differences, even if ntp shifts the system clock
        // a lot).
        //
        // NOT inline -- this would mean exposing hrFastTime.h, which
        // would be very bad for this hasty first release.
        //
	uint64 FastTime::getMicrosecondsSinceEpoch()
	{
            if (currentOffset != NULL)
            {
                if( usingHighRes )
                {
                    return static_cast<uint64>( HrFastTime::getSysSec() * 1.e6 );
                    //return static_cast<uint64>( HrFastTime::getProgSec() * 1.e6 );
                }
                else // revert to 'milliseconds' method (10 ms in HSEQ, 1ms in MTRACE?)
                {
                    return *currentOffset * (1000000U / frequency);
                }
            }
            else
            {
                return (util::getTime() - ptime(epoch)).total_microseconds();
            }
	}
	uint64 FastTime::getMicroseconds()
	{
            if (currentOffset != NULL)
            {
                if( usingHighRes )
                {
                    //return static_cast<uint64>( HrFastTime::getSysSec() * 1.e6 );
                    return static_cast<uint64>( HrFastTime::getProgSec() * 1.e6 );
                }
                else // revert to 'milliseconds' method (10 ms in HSEQ, 1ms in MTRACE?)
                {
                    return *currentOffset * (1000000U / frequency);
                }
            }
            else
            {
                return (util::getTime() - ptime(epoch)).total_microseconds();
            }
	}

} // end of namespace util

