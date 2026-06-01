/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

/** \file tscExtrapolator.h
 *  \brief A forward-correcting proportional-integral controller.
 */
#ifndef TSCEXTRAPOLATOR_H
#define TSCEXTRAPOLATOR_H

#include <iosfwd>
#include "seqlock.h"      // --- new util implementation-only file
#include "tscSupport.h"   // additional assembler stuff, main/util/

namespace util
{
    /**
     * This object is a simple proportional-integral controller
     * locking a forward predictor of system time to the time stamp
     * counter register.  One subtlety is that instead of applying
     * the correction immediately, the prediction is forced to be
     * piecewise continuous, with the current error "to be cancelled"
     * after a period of 'sleepTime'.
     *
     * When it loses lock, it tracks the cumulative time jumps
     * so that you can have progSec(ticks) guaranteed monotonic,
     * or sysSec(ticks) which follows the system time (generally
     * to within 100 us under normal conditions).  Loss of lock
     * can be caused by jumping system time forward or back by
     * more than half a second.
     *
     * It allows external access to the forward predictor by maintaining
     * an internal SeqLock.
     *
     * (It is also possible to slowly slew in the time jumps,
     *  regardless of how big they are.  I just haven't written
     *  it that way, in order to maintain a really good systime
     *  approximation.)
     */
    class TscExtrapolator //__attribute__((visibility=hidden))
    {
        friend class TextrapThread; // this guy (only) accesses the measure routine
        friend class HrFastTime;    // nonThread interface needs to 'measure()' things

        public:
            //protected: // someday?
            /** forward prediction function for system time:
             *
             *  This is monotonic when constructed or locked.
             *  Every time we detect a 'too big' jump in error,
             *  this guy jumps forward or back, instead politely
             *  slewing to the correct system time.
             */
            long double sysSec( uint64_t tick );

            /** forward prediction function for program time:
             *
             *  This is strictly monotonic.  It is sysSec with the
             *  cumulated total amount of time jumps added in.
             *  (Note, for accuracy, other formulation are possible,
             *   but this one is pretty simple).
             */
            long double progSec( uint64_t tick );

        private:
#if 0
            /** When driven by a thread with independent timers, our
             * internal 'measure' routine will take its own reading.
             *
             * \obsolete I think.
             *
             */
            uint64_t  readTicks( struct timespec &systemTime );
#endif

        public:
            /** sleepTime is rough time between recalibrating measurements.
             *
             *  memTime is rough indication of how far back we retain memory.
             *  (for smoothing out our tick rate estimates, we don't completely
             *   trust the new measurement, but mix in a bit of our old idea
             *   of the tick rate)
             *
             * 6 seconds is a compromise between time measurement
             * error and the time scale of frequency drift.
             *
             * This constructor should leave object unusable and requires
             * the client to initialize with either globalRate or individual
             * rates via some 'init(...)' routine.
             */
            TscExtrapolator( );

            /** This function is used to externally calculate and update model
             * parameters.  This interface is used for a higher-accuracy
             * mode (when it is determined to be applicable).
             *
             * In higher accuracy mode, measurements across all CPUs are used
             * to determine (externally) a single motherboard CPU-clocking
             * rate that is used for all TscExtrapolators in common.
             *
             * Doing this, all errors become systematic, so all CPUs will drift
             * in unison.  Relative timing errors are minimal, even between
             * measurements taken by different threads on different CPUs.
             *
             * It is also used in all modes to supply a reasonable set of
             * initialization data.
             */
            void init( uint64_t _bTick, struct timespec _b, double tickRate );

        protected:
            /**
             *  The update function is used (for now) by the threaded updater
             *  to test out montoring whether this motherboard supports
             *  microsecond-accuracy techniques.  (This is likely possible
             *  on 99.999% of all motherboards, if not more.)
             *
             *  (It is an empty function for now)
             */
            void update(
                    double ,//tickRate,
                    util::TscMeasurement const & //data
                    );

            /**
             *  New model update is supplied tick+time data.
             */
            void measure( util::TscMeasurement const &tscMeasurement,
                    std::ostream *os = NULL );

        private:
            /** b and bTick are the last raw measurement data */
            struct timespec b;
            uint64_t bTick;

            /** Current extrapolation: (fwd extrapolate, in a corrective
             *  manner from current idea of time at 'b')
             *  
             *  NOTE: bSec is some monotonically exptrapolated value, that
             *  in general will NOT agree with the measured value 'b'.
             *  We will try to compensate for the error during the
             *  next extrapolation period, so as to remain in synch
             *  with the system clock over long periods.
             */
            long double bSec;
            long double bHertz;

            // cum time jump is used to supply a guaranteed-monotonic
            // fwd predicted time
            long double cumJumpSec;

            long double cErrSum;
            long double cErrSumSqr;

        private:
            /** New stuff:  for external access, we will SeqLock read and
             * write to internal variables.  We will writelock (extremely
             * infrequently, like every 6 secons), and multiple readers
             * can use a SeqLock to read.  We will wrap all access to
             * automatically do the SeqLock business for you.
             *
             * The wrapper also has to do the apicId garbage to figure out
             * which one of us to read.
             */
            util::SeqLock seqlock;

    };

#ifdef TSCEX_MAIN
#define TSCEX_INLINE
#else
#define TSCEX_INLINE inline
#endif

    TSCEX_INLINE TscExtrapolator::TscExtrapolator( )
        : cumJumpSec( 0.0 )
        , cErrSum( 0.0 )
        , cErrSumSqr( 0.0 )
        {
            // NEW: constructor is empty, client decides on assumeCommonCPUclocks
            // and init's appropriately.
        }

    TSCEX_INLINE void TscExtrapolator::init( uint64_t _bTick, struct timespec _b, double tickRate )
    {
        this->bTick  = _bTick;
        this->b      = _b;
        this->bHertz = tickRate;
        this->bSec   = util::tdouble(this->b);
        // note: bSec is NOT the double value of b once we start updating the
        // model --- bSec is used to counteract current and cumulative errors.
        // b is raw data corresponding to bTick,
        // while bSec is predictor 'start value' and bHertz is predictor tick rate.
    }

} // util::
#undef TSCEX_INLINE

#endif // TSCEXTRAPOLATOR_H
