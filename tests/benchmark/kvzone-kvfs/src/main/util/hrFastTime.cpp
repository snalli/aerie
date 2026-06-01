/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

/** \file hrFastTime.cpp
 *  \brief API for a 'high-resolution' sub-module of boostTime.h
 */
//
// This file contains the independent-CPU version of time stamp counter
// calibration.  This makes no assumptions about sane motherboard
// engineering, and has completely independent time extrapolation for
// each CPU.  As a consequence, it's accuracy with 6 second update
// interval is typically under 200 microseconds.
// 
// This file contains the non-threaded interface, as driven from
// within util::FastTime.
//  
// (The threaded ::run routine has a bunch more code, to analyze if
//  a single global clock rate applies to all CPUs (in which case
//  timing drifts are systematic, not random, so relative time can
//  be measure to much better accuracy).
//
#define HRFT_MAIN // force some non-inlines
#include "hrFastTime.h"

#include "tscExtrapolator.h"
#include "hrFastTimeThread.h"
#include "tsc.h"
#include <util/types.h>




// debug:
//#define HRTRACE( stuff ) do { std::cout<< stuff; std::cout.flush(); } while(0)
#define HRTRACE( stuff ) do{;}while(0)
//#define HRTRACE( stuff ) do { LOG(INFO,stuff); }while(0);

#define NOTRACE( stuff ) do{;}while(0)

//
//
//     LOCAL HELPERS
//
//

#if 0
// quick estimate (not an example of high-performance timing)
static void __attribute__((unused)) // may disappear XXX
ticks4systime( uint64_t &lo, uint64_t &hi )
{
    lo = std::numeric_limits<uint64_t>::max();
    hi = 0ULL;
    struct timespec ts;

    {
        util::CpuCycler cc; // lock to "current" CPU.
        for( uint32 i=0U; i<10U; ++i )
        {
            util::flushPipeline();
            uint64_t a = util::rdtsc();
            // usually 7000-20000 ticks
            clock_gettime( CLOCK_REALTIME, &ts );
            util::flushPipeline();
            uint64_t b = util::rdtsc();
            lo = std::min( lo, b-a );
            hi = std::max( hi, b-a );
        }
        // we are still on original CPU
    }
    // relax cpu permissions again.
}
#endif


using util::uint32;
// NOT an example of how to do careful timing
static void __attribute__((unused)) // may disappear XXX
ticks4apicid( uint64_t &lo, uint64_t &hi )
{
    lo = std::numeric_limits<uint64_t>::max();
    hi = 0ULL;

    {
        util::CpuCycler cc; // convenient mechanism to lock to CPU temporarily
        for( uint32 i=0U; i<10U; ++i )
        {
            util::flushPipeline();
            uint64_t a = util::rdtsc();
            // Interesting: even with pipeline preflushed,
            // util::Apic::id() is 200-500 ticks
            // Note: this time measurement seems somewhat flaky, doesn't it?
            uint32_t volatile __attribute__((unused)) apic = util::Apic::id();
            util::flushPipeline();
            uint64_t b = util::rdtsc();
            lo = std::min( lo, b-a );
            hi = std::max( hi, b-a );
        }
    }
}

// NOT an example of how to do careful timing
static void __attribute__((unused)) // may disappear XXX
ticks4sampleall( uint64_t &lo, uint64_t &hi )
{
    std::vector< util::TscMeasurement > data( util::Apic::getNCpus() );
    lo = std::numeric_limits<uint64_t>::max();
    hi = 0ULL;

    {
        util::CpuCycler cc; // lightly paranoid -- sampleAllTscs returns us to same CPU set!

        // Throw away the first call -- it is expected to be slower.
        util::Apic::sampleAllTscs( &data[0] );
        for( uint32 i=0U; i<10U; ++i )
        {
            util::flushPipeline();
            uint64_t a = util::rdtsc();
            util::Apic::sampleAllTscs(&data[0]);
            util::flushPipeline();
            uint64_t b = util::rdtsc();
            lo = std::min( lo, b-a );
            hi = std::max( hi, b-a );
        }
    }
}

namespace util
{
    
} // util::

/** Let's get parCheck working with a fancier calibrator
 * (TscExtrapolator)
 *
 */

namespace util
{

    struct timespec HrFastTime::systime0;
    double HrFastTime::dsystime0 = 0.0;
    uint32_t HrFastTime::sleepTime = 0U;

    util::TextrapThread *HrFastTime::thread = NULL;
    util::TscExtrapolator *HrFastTime::textrap = NULL;
    util::TscMeasurement *HrFastTime::tmeasure = NULL;

    bool HrFastTime::initialized = false;
    bool HrFastTime::assumeCommonCPUclocks = false;
    //uint64_t HrFastTime::ticks4Systime = 0ULL;
    
    SeqLock HrFastTime::lockHrFastTimeUpdate;
    uint32 volatile HrFastTime::isHrFastTimeUpdateRunning = 0U;

    /** This is the only "write" operation to the high-res extrapolation data.
     * We use a write lock here, and readers will retry their operations
     * in case of conflicts with this code.
     */
    void HrFastTime::doUpdateExtrapolation()
    {
        LOG(INFO,"HrFastTime::doUpdateExtrapolation()...");
        smp_rmb(); // paranoia
        if( cas32( &isHrFastTimeUpdateRunning, 0U, 1U ) == 0U )
        {
            ScopedSeqWriteLock sswl( lockHrFastTimeUpdate );
            // Expect an extra 35-50 microseconds or so, every six seconds,
            // for a 4-CPU motherboard (server44)
            HrFastTime::updateExtrapolation(); // non-reentrant
            isHrFastTimeUpdateRunning = 0U;
            FastTime::isHrFastTimeUpdatePending = 0U;
            //smp_wmb(); //should be provided by destructor for sswl.
        }
        LOG(INFO,"HrFastTime::doUpdateExtrapolation() DONE");
    }

    // XXX TBD: lock around a previous-value to aid monotonicity ?
    //
    // monotonicity is guaranteed ONLY if you happen to be running
    // on the same CPU between the two calls.
    //
    // There is no easy way to force this, except by locking
    // (util::setAffinity) to a particular cpu before/after
    // requiring to measure exceedingly fast events.
    //
    // Note that blind use for short time intervals, if you
    // happen to context switch to a different CPU, will
    // have time difference measurement errors of at least
    // 200 microseconds, since per-CPU threads maintain their
    // own locks to sys time.
    //
    // On the same CPU, measurement accuracy for time intervals
    // is actually much better than the system lock time ---
    // the rate is OK to a part in a million or better, so
    // I'd expect microsecond accuracy for times < 1 second,
    // based on long running tests of clock rate drifts and
    // measurement errors.
    //
    long double HrFastTime::getSysSec()
    {
        HRTRACE("HrFastTime::getSysSec(), update? "<<FastTime::isHrFastTimeUpdatePending);
        // Now the signal handler merely sets a flag to tell us that
        // it is suggested to update the high-res timing data (roughly
        // every six seconds if we are regularly making use of the high-res
        // timers).
        //
        // At some point, approximately every 6 seconds or longer,
        // exactly one invocation of me will to update the timers.
        //
        // Multiple threads may concurrently read/extrapolate times, but not
        // if we have a tsc update in progress. This is a fast operation,
        // so using a sequence lock is appropriate.  (Those rare reads that
        // conflicted will be retried)
        //
        // So SeqLock is an appropriate multiple-reader-single-writer lock.
        //
        
        if( FastTime::isHrFastTimeUpdatePending )
        {
            HrFastTime doUpdateExtrapolation(); // write-locking lockHrFastTimeUpdate
        }

        DASSERT( HrFastTime::initialized, "Failed to HrFastTime::startExtrapolators(..)" );
        DASSERT( HrFastTime::textrap != NULL, "Time extrapolators not present" );
        uint32_t begApic;
        uint32_t endApic;
        long double ret;
        do // until we think we have run this block on a single CPU, usually once.
        {
            // clock_gettime is about 8000 cycles (4 us)
            // full version (with util::Apic::id etc.) is 1200-1500 ticks

            SeqReadLock srl( lockHrFastTimeUpdate );
            do // until we have a reading that has not conflicted with a timer write operation
            {  // (multiple readers are allowed to execute this code block)
                begApic = util::Apic::id();
                srl.reading();
                uint32_t begCpu = util::Apic::cpu( begApic );

                uint64_t tscTicks = util::rdtsc();
                ret = HrFastTime::textrap[begCpu].sysSec( tscTicks );
                //                                ^^^^^^
            }
            while( srl.retry() );

            endApic = util::Apic::id();
        }
        while( endApic != begApic );

        return ret;
    }

    long double HrFastTime::getProgSec()
    {
        HRTRACE("HrFastTime::getProgSec(), update? "<<FastTime::isHrFastTimeUpdatePending);
        // except for a call to progSec instead of sysSec, this
        // is identical to the previous function -- it shares the
        // write/read locking variables too.

        if( FastTime::isHrFastTimeUpdatePending )
        {
            HrFastTime doUpdateExtrapolation(); // write-locking lockHrFastTimeUpdate
        }

        DASSERT( HrFastTime::initialized, "Failed to HrFastTime::startExtrapolators(..)" );
        DASSERT( HrFastTime::textrap != NULL, "Time extrapolators not present" );
        uint32_t begApic;
        uint32_t endApic;
        long double ret;
        do // until we think we have run this block on a single CPU.
        {
            SeqReadLock srl( lockHrFastTimeUpdate );
            do // until we have not conflicted with an update (write) operation
            {
                begApic = util::Apic::id();
                srl.reading();
                uint32_t begCpu = util::Apic::cpu( begApic );

                uint64_t tscTicks = util::rdtsc();
                ret = HrFastTime::textrap[begCpu].progSec( tscTicks );
                //                                ^^^^^^^
            }
            while( srl.retry() );

            endApic = util::Apic::id();
        }
        while( endApic != begApic );
        return ret;
    }

    // this is the nonthreaded update routine.
    void HrFastTime::updateExtrapolation()
    {
        PASSERT( HrFastTime::thread == NULL,"Please do not updateExtrapolation."
                "\nYou have asked for a thread to do the updating for you." );
        if( HrFastTime::initialized )
        {
#if 0
            struct timespec ts;
            clock_gettime( CLOCK_REALTIME, &ts );
            cout<<" nonThread update at "<<util::tdouble(ts) - dsystime0<<endl;
#endif
#if 1
            // just to point out that the initial implementation will always
            // use the motherboard-from-hell model...
            if( HrFastTime::assumeCommonCPUclocks == true )
            {
                HrFastTime::assumeCommonCPUclocks =  false;
            }

            if( HrFastTime::assumeCommonCPUclocks == false )
            {
                // XXX temporary... eventually interface will always
                //     use the Apic::sample* routine, and changes to
                //     what gets measured and passed around will occur.
                //
                //
                //
                //
                //
                // use this interface to update each independently (for now)
                // this is the original bad-motherboard version of the extrapolator
                // This code IS able to handle badness such as time jumps,
                // (by recording them, for now)
                uint32_t cpus = util::Apic::getNCpus();

                // No -- can only use static or stack memory since this might
                // run like an interrupt routine.
                //static util::TscMeasurement * tmeasure = new util::TscMeasurement[ cpus ];
                //
                util::Apic::sampleAllTscs( HrFastTime::tmeasure );

                for( uint32_t cpu=0U; cpu<cpus; ++cpu )
                {
                    HrFastTime::textrap[cpu] .measure( tmeasure[cpu] );
                }
            }
#endif
        }
    }


    void util::HrFastTime::stopExtrapolation()
    {
        ScopedSeqWriteLock sswl( lockHrFastTimeUpdate );

        if( HrFastTime::thread != NULL )
        {
            HRTRACE(" joining tsc updater thread...");
            HrFastTime::thread->join();
            //cout<<endl;

            HRTRACE(" deleting updater thread...");
            delete HrFastTime::thread;
            HrFastTime::thread=NULL;
        }
        
        HRTRACE(" deleting extrapolators...");
        delete[] HrFastTime::textrap;
        HrFastTime::textrap = NULL;
        delete[] HrFastTime::tmeasure;
        HrFastTime::tmeasure = NULL;

        HRTRACE(" misc other variable resets...");
        HrFastTime::sleepTime = 0U;
        HrFastTime::initialized = false;
        HRTRACE("stopExtrapolation DONE"<<endl);
    }

    // XXX This start routine and the threaded ::run contain similar
    // code to determine whether or not we can assume a global clock rate
    // or not. XXX  should be rewritten when the actual decision making
    // algorithm is fully decided.
    void util::HrFastTime::startExtrapolation(
            uint32_t _sleepTime /*seconds*/,
            bool needThread
            )
    {
        PASSERT( ! HrFastTime::initialized, "duplicate attempt to initialized HrFastTime" );
        HRTRACE("util::Apic::init()"<<endl);
        util::Apic::init(); //just in case it hasn't been
        HRTRACE("Acquiring initial tick rate estimates"<<endl);

        // safe, but if everything else is correct it's overkill:
        ScopedSeqWriteLock sswl( lockHrFastTimeUpdate );

        HrFastTime::sleepTime = _sleepTime;
        //HrFastTime::nCpus = getNCpus();
        uint32_t cpus = util::Apic::getNCpus();

        clock_gettime( CLOCK_REALTIME, &HrFastTime::systime0 );
        HrFastTime::dsystime0 = util::tdouble( HrFastTime::systime0 );
        HRTRACE("'Prog' seconds reported wrt. start <main-start-time> = "<<dsystime0<<endl);

        // NEW: this guy constructs fast but is uninitialized, waiting for
        // us to 'init'
        PASSERT( HrFastTime::textrap == NULL, "Ohoh.  textrap array already existed" );
        HrFastTime::textrap = new util::TscExtrapolator [ cpus ];

        // Provide this static work area for updating during "interrupt
        // handler" settings (no malloc/new allowed).
        PASSERT( HrFastTime::tmeasure == NULL, "Ohoh.  tmeasure array already existed" );
        HrFastTime::tmeasure = new util::TscMeasurement [ cpus ];

        // try a few times to initialize things with a reasonable system
        // timer.
        std::vector< util::TscMeasurement > begData( cpus );
        std::vector< util::TscMeasurement > endData( cpus );
        uint32_t const attempts=3U;
        bool badTime = false;
        bool impossiblyBadTime = false;
        for( uint32_t attempt=0U; attempt<attempts; ++attempt )
        {
            // take an initial quick sampling of all timer data.
            //HrFastTime::sampleAllCpus( &begTicks[0], &begSecs[0], cpus );
            util::Apic::sampleAllTscs( &begData[0] );

            sleep(1);

            //HrFastTime::sampleAllCpus( &endTicks[0], &endSecs[0], cpus );
            util::Apic::sampleAllTscs( &endData[0] );

            // are all system time calls somewhere around 1 sec in future?
            for( uint32_t cpu=0U; cpu< cpus; ++cpu )
            {
                double secs = tdiff( endData[cpu].time, begData[cpu].time );
                if( secs <= 0.0 )
                {
                    impossiblyBadTime = true;
                }
                if( secs < 0.5 || secs > 1.5 )
                {
                    badTime = true; // try again.
                }
            }

            if( badTime || impossiblyBadTime )
            {
                if( attempt < attempts-1 ) continue; // silent retry 1st few times

                if( impossiblyBadTime )
                {
                    HRTRACE("Oh-oh, system time is repeatedly going backwards"
                            "\n I cannot initialize HrFastTime subsystem properly"<<endl);
                    // FATAL !
                    exit(-1);
                }
                if( badTime )
                {
                    // non-silent complaint, but we'll do our best, forcing !assumeCommonCPUclocks
                    HRTRACE("Oh-oh, sleep(1) gave wild results.  Rates had relative error"
                            "\nlarger than "<<HRFASTTIME_MAXRELDEVN_TARGET<<" Hertz, I'll initialize"
                            "\nHrFastTime without assuming common CPU clocks.  This is"
                            "\n the least accurate, but most general timing model."<<endl);
                    HrFastTime::assumeCommonCPUclocks = false;
                }
            }
            else
            {
                HRTRACE("Good, found some reasonable intial time data"<<endl);
            }
                        

        } // end attempts to find a good 'sleep' interval for initialization data

        double globalTickRate = 0.0; // not available yet

        if( ! ( badTime || impossiblyBadTime ) )
        {
            // Analyze begTicks,begSecs, endTicks,endSecs
            //
            // Compare average tick rates over this period and decide
            // whether we should assume common clock rate at all CPUs or not.
            

            // are ticks per second roughly equivalent?
            // What is max deviation from linear relation?
            //
            // XXX BEST WAY: during startup, measure for 1 sec,
            // get an overdetermined set of linear equations for
            // Seconds[0..4] = (5x4 matrix) * Ticks[0..4]
            // and check, statistically, that the 4 matrix entries
            // for rates are equivalent (... and if you want, check
            // that the offsets are equivalent).
            // (Unfortunately, svd is only available on our boxes
            // via libgsl, as far as I can see)
            //
            // For now, I will use a simpler method: if all rates
            // are equivalent to some fraction I'll assume a common
            // clock rate at all CPUs.
            //
            // What to do?  Now the model has only nCpus+1 parameters:
            // nCpus tick offsets, and a single global tick rate.
            //
            // This means that at every "en masse" update, I can do
            // a light massage of the offsets, and a global setting
            // for the global tick rate in such a way that errors
            // with respect to drifts of actual clock rate affect
            // all extrapolators in common.
            //
            // The net result is that time differences are accurate
            // to within the offset measurement accuracy.  I think
            // the offset measurement accuracy can typically be
            // held to a microsecond or so (i.e. tick +/- 2000).
            //
            // Why so low error?
            // Since all raw measurements can occur [typically]
            // within 35 microseconds [so rate error over 35 microseconds
            // is about zero, and relative offsets are determined
            // with accuracy at some small fraction of the time
            // it takes for the system time measurement [~ 4 us]
            double avgRate = 0.0;
            double maxRate = std::numeric_limits<double>::min();
            double minRate = std::numeric_limits<double>::max();
            double avgSecs = 0.0;
            for(uint32_t cpu=0U; cpu < cpus; ++cpu )
            {
                HRTRACE( " cpu "<<cpu
                        <<" start "<<begData[cpu].tick
                        <<','<<util::tdouble(begData[cpu].time)-HrFastTime::dsystime0
                        <<" end "<<endData[cpu].tick
                        <<','<<util::tdouble(endData[cpu].time)-HrFastTime::dsystime0
                        <<endl);
                // Oh, not the most precise way to calculate, but good enough for now.
                double secs = tdiff(endData[cpu].time, begData[cpu].time);
                avgSecs += secs;
                double rate = (endData[cpu].tick - begData[cpu].tick) / secs;
                avgRate += rate;
                maxRate = std::max( maxRate, rate );
                minRate = std::min( minRate, rate );
            }
            avgRate *= (1.0/cpus);
            avgSecs *= (1.0/cpus);
            double maxRelDevn = (maxRate - minRate) / avgRate;
            // Now the important motherboard from hell decision:
            HRTRACE("\n\nOver the same ~ "<<avgSecs<<" second period"
                    "\nclock rate max relative deviation between CPUs was "
                    <<maxRelDevn<<"\n, with rates in range [ "<<minRate
                    <<" , "<<maxRate<<" ]\n\n");
            if( maxRelDevn < HRFASTTIME_MAXRELDEVN_TARGET )
            {

                globalTickRate = avgRate;
                HrFastTime::assumeCommonCPUclocks = true;
                /***************************************/

                HRTRACE("\n\nExcellent, we will assume that all CPUs are driven"
                        "\nby a common clock.  This should make relative times"
                        "\nacross CPUs accurate to within a few microseconds.\n\n"
                       );
            }
            else
            {

                HrFastTime::assumeCommonCPUclocks = false;
                /****************************************/

                HRTRACE(" Oh no. CPUs on this motherboard have widely varying"
                        "\nclock rates.  This means our TSC model will be"
                        "\nfully independent amongst all CPUs, and you can"
                        "\nuncorrelated time estimate errors as high as +/- 200 us."
                        "\nThis may make very short relative times between"
                        "\nevents on different CPUs appear to be negative!!!"
                       );
            }
        } // end of global clock rate analysis.

        HRTRACE("Initializing all Textrapolators with assumeCommonCPUClocks = "
            <<HrFastTime::assumeCommonCPUclocks
            <<" globalTickRate="<<globalTickRate<<endl);

        for( uint32_t cpu=0U; cpu<cpus; ++cpu )
        {
            double rate;
            if( HrFastTime::assumeCommonCPUclocks )
            {
                rate = globalTickRate;
            }
            else
            {
                // individual tick rates for each extrapolator
                double secs = tdiff(endData[cpu].time, begData[cpu].time);
                rate = (endData[cpu].tick - begData[cpu].tick) / secs;
                HRTRACE(" cpu "<<cpu<<" secs="<<secs<<" rate="<<rate<<endl);
            }

            HRTRACE(" Initializing extrapolator for cpu "<<cpu<<endl);
            textrap[cpu].init( endData[cpu].tick, endData[cpu].time,    rate );
            /*               raw measurement, raw measurement, model */
        }

        if( needThread )
        {
            //cout<<" constructing a TextrapThread updater thread...";
            HrFastTime::thread = new util::TextrapThread( sleepTime );
            //cout<<endl;

            //cout<<" starting updater thread...";
            HrFastTime::thread->start();
            //cout<<endl;
        }
        else
        {
            HrFastTime::initialized = true;
        }
    }

} // util
