/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 *
 * Helper functions/classes for boost_date_time library.
 */ 
 
#ifndef _UTIL_BOOSTTIME_H
#define _UTIL_BOOSTTIME_H

#include <time.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "util/exceptions.h"
#include "util/debug.h"
#include "util/misc.h"
#include "util/statistics.h"
#include "util/types.h"

//============================================================================

namespace util
{
    using boost::posix_time::time_duration;
    using boost::posix_time::ptime;
    using boost::xtime;
    using namespace boost::gregorian;

    /**
     * Converts time_duration to the number of seconds.
     */
    uint64 timeDurationInSeconds(time_duration const& duration);

    /**
     * Converts time_duration to the number of milliseconds.
     */
    uint64 timeDurationInMillisecs(time_duration const& duration);

    /**
     * Converts time_duration to the number of microseconds.
     */
    uint64 timeDurationInMicrosecs(time_duration const& duration);

    /**
     * Converts time_duration to the number of nanoseconds.
     */
    uint64 timeDurationInNanosecs(time_duration const& duration);

    /**
     * Converts the number of fractional seconds in time_duration
     * to milliseconds. Seconds, minutes and hours are not
     * included!!
     */
    uint32 timeDurationMillisecs(time_duration const& duration);

    /**
     * Converts the number of fractional seconds in time_duration
     * to microseconds. Seconds, minutes and hours are not
     * included!!
     */
    uint32 timeDurationMicrosecs(time_duration const& duration);

    /**
     * Converts the number of fractional seconds in time_duration
     * to nanoseconds. Seconds, minutes and hours are not
     * included!! Note, the underlying boost time classes
     * may not operate in nanosec resolution.
     */
    uint32 timeDurationNanosecs(time_duration const& duration);

    /**
     * Converts time_duration to number of fractional seconds.
     * The result may overflow!
     * 
     * Note: timeDurationInMillisecs, etc. might be easier
     * to use.
     * 
     * @param TRes     The resolution, e.g. 10^3 for millisecs, 10^6 for microsecs,
     *                 10^9 for nanosecs.
     * @param duration The time duration.
     * 
     * @return The duration expressed in fractional seconds.
     */
    template <uint32 TRes>
        uint64 timeDurationInFractionalSeconds(time_duration const& duration);

    /**
     * Converts the fractional seconds PART of time_duration to 
     * number of fractional seconds in specified resolution.
     * 
     * Seconds, minutes and hours are not included! 
     * 
     * @param TRes    The resolution, e.g. 10^3 for millisecs, 10^6 for microsecs,
     *                 10^9 for nanosecs.
     * @param duration The time duration.
     * 
     */
    template <uint32 TFact>
        uint32 timeDurationFractionalSeconds(time_duration const& duration);

    /**
     * Converts time_duration to timespec.
     * 
     * @param duration The duration.
     * 
     * @return The timespec structure.
     */
    timespec timeDuration2TimeSpec(time_duration const& duration);


    /**
     * Converts time_duration to timeval.
     * 
     * @param duration The duration.
     * 
     * @return The timeval structure.
     */
    timeval timeDuration2TimeVal(time_duration const& duration);

    /**
     * Converts time_duration to xtime.
     * 
     * @param duration The duration.
     * 
     * @return The xtime structure.
     */
    xtime timeDuration2XTime(time_duration const& duration);


    /**
     * Converts ptime to xtime.
     * 
     * @param pt Time expressed as ptime. 
     * 
     * @return Time expressed as xtime.
     */
    xtime ptime2xtime(ptime const& pt);

    /**
     * Less-then operator for boost thread' xtime.
     */
    bool operator<(xtime const& t1, xtime const& t2);

    bool operator==(xtime const& t1, xtime const& t2);

    xtime operator+(xtime const& t1, xtime const& t2);

    xtime& getUniversalTimeWithOffset(xtime& result, xtime const& offset);

    xtime getUniversalTimeNow();

    /**
     * getTime returns current time (in ptime format).
     * It works like: microsec_clock::local_time() but is faster.
     * The function is implemented and used since performance.
     **/
    ptime getTime();

    //============================================================================

    //
    // Inline functions definitions below
    //

    template <uint32 TRes>
        inline uint64 timeDurationInFractionalSeconds(time_duration const& duration)
        {
            return duration.hours() * 3600 * TRes + 
                duration.minutes() * 60 * TRes +
                duration.seconds() * TRes +
                duration.fractional_seconds() * TRes / time_duration::traits_type::res_adjust();
        }

    template <uint32 TRes>
        inline uint32 timeDurationFractionalSeconds(time_duration const& duration)
        {
            return duration.fractional_seconds() * TRes / time_duration::traits_type::res_adjust();
        }

    inline bool operator<(xtime const& t1, xtime const& t2)
    {
        return t1.sec < t2.sec || (t1.sec == t2.sec && t1.nsec < t2.nsec);
    }

    inline bool operator==(xtime const& t1, xtime const& t2)
    {
        return t1.sec == t2.sec && t1.nsec == t2.nsec;
    }

    inline xtime operator+(xtime const& t1, xtime const& t2)
    {
        xtime result;

        result.sec = t1.sec + t2.sec;
        result.nsec = t1.nsec + t2.nsec;

        const int32 MILIARD = 1000000000;

        if (result.nsec >= MILIARD)
        {
            result.sec += 1;
            result.nsec -= MILIARD;
        }

        return result;
    }

    inline xtime& getUniversalTimeWithOffset(xtime& result, xtime const& offset)
    {
        int32 error = xtime_get(&result, boost::TIME_UTC);
        util::unused(error);
        DASSERT(error == boost::TIME_UTC, "error in getting UTC time");
        result = result + offset;
        return result;
    }

    inline xtime getUniversalTimeNow()
    {
        xtime result;
        int32 error = xtime_get(&result, boost::TIME_UTC);
        util::unused(error);
        DASSERT(error == boost::TIME_UTC, "error in getting UTC time");
        return result;
    }

    static date epoch(1970, 1, 1);

    inline ptime getTime()
    {
        struct timeval tv;
        struct timezone tz;

        gettimeofday(&tv, &tz);

        // this assertion is to prevent default time_duration resolution changes.
        // current implementation implies that a fourth argument to time_duration should be microseconds
        BOOST_STATIC_ASSERT(sizeof(time_duration) == 8);

        return ptime(epoch, time_duration(0,0, tv.tv_sec, tv.tv_usec));
    }

    class HrFastTime; // opaque fwd decl;

    class FastTime
    {
        friend class HrFastTime; // can read and reset isHrFastTimeUpdatePending variable
        public:
        /**
         * Start fast time which is using SIGALRM to increment virtual time "frequency" times during one second.
         * Time resolution is 1000/"frequency" milliseconds.
         * @param timeOutOfSynch - callback function which will be called when time desynchronization has been noticed.
         *          NOTE: This should be a static function which does not allocate memory and does not obtain any locks.
         *          It can be called from within any other function (during signal handling procedure).
         *
         * @param highres - Default true.  Set to false to avoid all interaction with the
         *                  microsecond timing submodule.  When false, microseconds calls
         *                  will have revert to millisecond (fastest) times.  There is no
         *                  way (provided) to turn on the highres time module later.
         *
         */
        static void startFastTimer(void (*timeOutOfSynchArg) (time_duration const &), bool highres = true);
        static void startFastTimer( bool highres = true);
        /**
         * Cleanup at the end - should not be called when multiple threads are still invoking getTime() function.
         */
        static void finishFastTimer();
        /**
         * Return fast time if it was initialized with startFastTime() function - otherwise util::getTime()
         */
        static ptime getTime();
        /**
         * Return fast time if it was initialized with startFastTime() function - otherwise util::getTime()
         */
        static uint64 getMillisecondsSinceEpoch();

        /**
         * Return HrFastTime time if it was initialized with startFastTime() function - otherwise util::getTime()
         *
         * HrFastTime is an experimental module that in early form monitors and reads time stamp
         * counter registers to provide higher resolution.  The initial implementation always uses
         * extremely conservative assumptions and provides high precision, but only around +/- 200
         * microsecond accuracy (usually it's much better accuracy).  This is useful if your timing
         * measurement does averaging, so that accuracy can be increased to approach the measurement
         * precision.
         *
         * Absolute accuracy at microsecond level, if system monitoring indicates it is possible on
         * your motherboard is TBD.
         * 
         * Current highres resolution is microseconds, and long-term accuracy is something
         * like 33 us +/- 200 us (typical over 24 hours).
         *
         *
         * We provide raw values, so that negative time jumps are possible (this is required if
         * averaging is to give a good value).
         *
         * For now, the entire HrFastTime sub-module is "hidden detail".
         *
         * I think for best application to getting delta times, I will return a time that is
         * strictly increasing per CPU, but will deviate from system time if system time jumps.
         * So use these results only for averaging with other like results.
         *
         * Return type is 'double' so you should get warning if you try to equate this time
         * with time gotten by other mechanisms.   (For now).
         *
         * Microsecond timers are turned off if the constructor \c highres argument
         * is supplied and is false.  When off, we will return a milliseconds
         * value * 1000U.
         *
         */
        static uint64 getMicrosecondsSinceEpoch();

        /**
         * The high resolution submodule naturally approximates both "system" time
         * and a monotonic "program" time
         *
         * Program time should go forward even if system clock jumps back
         * discontinuously when system time slows down by "unreasonable"
         * amounts.
         *
         * Several highres timing application are more concerned with accurate
         * delta-times, so "program" time is made available for such clients.
         *
         * \note Because of measurement precision issues, it is
         * also advisable that clients protectively calculate *signed*
         * delta times before generating average times!
         */
        static uint64 getMicroseconds();

        /** getter, for unittests and debug */
        static bool getIsHrFastTimeUpdatePending();

        /**
         * Return time update frequency. Similar to time resolution.
         */
        static time_duration getFrequency();

        private:
        static void initialize(bool highres);
        static void destroy();
        // signal handling procedure
        static void incrementTime(int32);
        // frequency measure in ticks per second
        static int32 const frequency = 100;
        //util::CTAssert<(1000 % frequency == 0)> frequencyMustDivide1000;
        // current time is stored under this pointer (in milliseconds)
        static uint64 volatile * volatile currentOffset;
        // two time variables to avoid race condition on uint64 incrementation
        static uint64 volatile offset1;
        static uint64 volatile offset2;
        // static callback function which will be called every time fast time is out of synch with the current time
        // negative argument means our time is ahead of system time
        // positive means our time is before system time
        static void (* timeOutOfSynch) (time_duration const & );
        // external flag, set in timer signal handler, reset in hrFastTime.cpp
        static bool volatile isHrFastTimeUpdatePending;
    };

    inline ptime FastTime::getTime()
    {
        if (currentOffset != NULL)
        {
            return boost::posix_time::ptime(epoch, boost::posix_time::milliseconds(*currentOffset * (1000 / frequency)));
        }
        else
        {
            return util::getTime();
        }
    }

    inline uint64 FastTime::getMillisecondsSinceEpoch()
    {
        if (currentOffset != NULL)
        {
            return *currentOffset * (1000 / frequency);
        }
        else
        {
            return (util::getTime() - ptime(epoch)).total_milliseconds();
        }
    }

    inline ptime getFastTime()
    {
        return FastTime::getTime();
    }

    inline bool FastTime::getIsHrFastTimeUpdatePending()
    {
        return FastTime::isHrFastTimeUpdatePending;
    }

} // end of namespace util

#endif  // _UTIL_BOOSTTIME_H

