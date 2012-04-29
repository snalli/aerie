/*
 * Copyright (c) 2007 NEC Laboratories, Inc. All rights reserved.
 */

/** \file hrFastTimeThread.cpp
 *  \brief threaded test/dev interface to HrFastTime
 *
 *  \note this thread interface is used by utilities and for development
 *  of new timing algorithms -- it is allowed (even expected) to be verbose!
 */
#include <iostream>
#include <iomanip>
#include <boost/bind.hpp>

#include "hrFastTimeThread.h"
#include "hrFastTime.h"
#include "tscExtrapolator.h"
#include "tsc.h"
#include "tscSupport.h"

using namespace std;

// debug:
#define HRTRACE( stuff ) do { std::cout<< stuff; std::cout.flush(); } while(0)
//#define HRTRACE( stuff ) do{;}while(0)

#define NOTRACE( stuff ) do{;}while(0)

namespace util
{
    TextrapThread::TextrapThread(uint32_t ,//_cpu
            uint32_t //_sleepTime
            )
        {
        }

    TextrapThread::~TextrapThread()
    {
    }

    void TextrapThread::join()
    {
        this->thrd->join();
    }

    void TextrapThread::start()
    {
        this->thrd = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&TextrapThread::run, this)));

    }

    void TextrapThread::run()
    {
        uint32_t cpus = util::Apic::getNCpus();
        HRTRACE("\tRunning TscExtrapolators every "
            <<HrFastTime::sleepTime<<" seconds "
            " for "<<cpus<<" CPUs"<<endl);

        vector< util::TscMeasurement > tscData( util::Apic::getNCpus() );
        //std::vector< uint64_t > ticks( cpus );
        //std::vector< struct timespec > times( cpus );

        cout<<setprecision(20);

        for(;/*ever*/;)
        {
            boost::xtime xt;
            boost::xtime_get (&xt, boost::TIME_UTC);
            xt.sec += HrFastTime::sleepTime;
            sleep(xt);

            double globalTickRate = 0.0;
            // if following block exits with globalTickRate still 0.0,
            // we will assume that it failed, and do the "assume nothing" update
            
            if( HrFastTime::assumeCommonCPUclocks == true )
            {
                NOTRACE( "XXX COMMON CLOCKS"<<endl );
                // cpus time clocks are assumed to be synchronized, derived from a common
                // motherboard crystal, so rate variations are a global thing.
                //
                // In this case I take all samples as fast as possible, to easily get
                // decent values for rate-corrected tick offsets among the CPUs.  These
                // tick offsets should vary quite slowly over time, with the major
                // variation being the global CPU clock rate.  Ideally, if we
                // really believe the offsets are identical to zero, we can avoid
                // checking the cpuid at all (FUTURE WORK: flag this state and
                // implement this optional speedup -- it seems to be good on my
                // server box :)

                // locking NOTE: if I sample and update separately, then I
                // am not write-locking the extrapolator, so 'old' extrapolation
                // MIGHT be used before new monotonic values are in place.
                // It is possible to post-correct with a post-lock re-rdtsc()
                // (fast) and some horrid math.   But easiest is to ignore this
                // since it is likely to be a very small difference between old
                // and new predictors over the small unlocked time period.
                //
                // (start point is guaranteed common, so even a big rate difference
                //  of 1e6 MHz is a 1 ns error if we get delayed by a whopping
                //  millisecond between measurement and update).
                //
                // 1 ns error from this is the least of my worries.

                //HrFastTime::sampleAllCpus( &ticks[0], &times[0], cpus );
                // 'double' version of times is good enough for now :(
                util::Apic::sampleAllTscs( &tscData[0] );

                // XXX TBD: move this calc somewhere, it is useful enough
                // to avoid rewrites (and also the test for rate equality)
                double avgRate = 0.0;
                double maxRate = std::numeric_limits<double>::min();
                double minRate = std::numeric_limits<double>::max();
                double avgSecs = 0.0;
                for( uint32_t cpu=0U; cpu<cpus; ++cpu )
                {
                    // can be -ve if sys time jumped backward...
                    //
                    // If any CPU indicates even a possibility of system time jump,
                    // then we are safer to return right away to unsafe mode:
                    //
                    // If system time is unstable, my view is that "all bets are off".
                    // Certainly, I would not wish to assume that all CPU extrapolators
                    // were affected similarly by a time jump.
                    //
                    double secs = util::tdouble(tscData[cpu].time) - util::tdouble(HrFastTime::textrap[cpu].b);
                    double rate = (tscData[cpu].tick - HrFastTime::textrap[cpu].bTick)
                        / secs;
                    if( rate < 1.e6 )
                    {
                        // definite sign of big trouble.
                        HRTRACE("\nOh no. clock rate of under 1MHz means something"
                                "\nis very wrong.  time extrapolators are now wild.");
                        // say that we no longer have common CPU clocks, because
                        // that at least has some recovery code that will log
                        // time jumps (it is likely a big negative time jump that
                        // has gotten us here.)
                        HrFastTime::assumeCommonCPUclocks = false;

                        // THIS is one of those places a goto is tempting.
                        // XXX break into subroutines so logic is more clear. XXX
                            
                        break;
                    }

                    avgSecs += secs;
                    avgRate += rate;
                    maxRate = std::max( maxRate, rate );
                    minRate = std::min( minRate, rate );
                }
                // if we still think everything might be alright....
                if( HrFastTime::assumeCommonCPUclocks == true )
                {
                    NOTRACE(" XXX STILL COMMON CLOCKS"<<endl);
                    avgRate *= (1.0/cpus);
                    avgSecs *= (1.0/cpus);
                    double maxRelDevn = (maxRate - minRate) / avgRate;
                    if( avgSecs > 0.0 && avgRate > 1.e6
                            && maxRelDevn < HRFASTTIME_MAXRELDEVN_TARGET )
                    {
                        // Great! We can do a high-accuracy update of the
                        // extrapolators.
                        globalTickRate = avgRate;
                        NOTRACE(" XXX STILL STILL COMMON CLOCKS globalTickRate="<<globalTickRate<<endl);
                    }
                    else
                    {
                        HRTRACE("\n\nOver ~ "<<avgSecs<<" second period"
                                "\nclock rate max relative deviation between CPUs was "
                                <<maxRelDevn<<"\n, with rates in range [ "<<minRate
                                <<" , "<<maxRate<<" ]\n\n");
                        HRTRACE(" SWITCHING TO FAILSAFE MODE (rel. times more inaccurate)");
                        HrFastTime::assumeCommonCPUclocks = false;
                    }
                }
            }

            // If we have permanent or transient clock unsynch, or systime
            // jump... switch to the nothing-assumed model and retry
            // the entire update (this code is only place I deal/record
            // system time jumps, for now XXX)
            if( HrFastTime::assumeCommonCPUclocks == false )
            {
                // this is the original bad-motherboard version of the extrapolator
                // This code IS able to handle badness such as time jumps,
                // (by recording them, for now)
                uint32_t cpus = util::Apic::getNCpus();
                static util::TscMeasurement * rawData = new util::TscMeasurement[ cpus ];
                util::Apic::sampleAllTscs( rawData );
                for( uint32_t cpu=0U; cpu<cpus; ++cpu )
                {
                    cout<<"cpu "<<cpu;
                    HrFastTime::textrap[cpu] .measure( rawData[cpu], &cout );
                }
            }
        }
        util::setAffinity(-1); // next time, I'll start on any old CPU
    }

}
