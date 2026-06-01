/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

/** \file tsc.cpp
 *  \brief APIC-related utilities, including sample-time-data across all CPUs.
 */

#include "tsc.h"                // The most important stuff.
#include "tscSupport.h"         // Gory helpers.

#include <util/debug.h>

#include <iostream>
#include <boost/limits.hpp>


// lukasz suggested twenty, I suggested two, this is my compromise.
//
// (I should be able to rewrite the modellers to explictly deal
//  with slightly less precise data measurements.  An accuracy
//  estimate is now part of the returned TscMeasurement struct.)
#define MAX_PRECISE_TSC_MEASUREMENT_ATTEMPTS 3U

// rewriting something? something is broken?
//#define MYTRACE( stuff ) do{ cout<< stuff ; cout.flush(); }while(0)
#define MYTRACE( stuff ) do{ LOG(util::DEBUG,stuff); }while(0)
//#define MYTRACE( stuff ) do{;}while(0)

using namespace std;

namespace util
{

    // it is highly unlikely that your APIC ids go this high.
#define SOME_UNAVAILABLE_CPU_ID ((uchar)(0xFF))

    //
    //
    //    initialize data members
    //
    //

    static uint64_t ticks4systime();
    uint64_t const Apic::ticks4Systime = ticks4systime();

    uchar  Apic::apic2cpu[256] = { SOME_UNAVAILABLE_CPU_ID };

    // initialized during static_initialization_and_destruction routine
    uint32_t const Apic::nCpus
        = get_nprocs() // GNU extension: sys
        //= sysconf(_SC_NPROCESSORS_CONF) // let's assume all are available
        ;

    //
    //
    //   static helpers
    //
    //


    static inline void takeAReading( struct TscMeasurement *x )
    {
        uint64_t const maxSyscallTime = Apic::getTicks4Systime() << 1U;

        uint64_t a, b;
        struct timespec ts;

        // current measurement precision = worst possible.
        x->measurementTicks = std::numeric_limits<uint64_t>::max();

        for( uint32_t attempt=0U; attempt < MAX_PRECISE_TSC_MEASUREMENT_ATTEMPTS; ++attempt)
        {
            a = util::rdtsc();
            clock_gettime( CLOCK_REALTIME, &ts );
            b = util::rdtsc();
            if( (b-a) < x->measurementTicks ) // precision improved?
            {
                x->measurementTicks = b - a;
                x->tick = (x->measurementTicks >> 1U) + a;
                x->time.tv_sec = ts.tv_sec;
                x->time.tv_nsec = ts.tv_nsec;
                // is precision reasonable enough?
                if( x->measurementTicks < maxSyscallTime )
                {
                    break; // good enough precision
                }
            }
        }
    }

    //
    //
    //   implementation
    //
    //

    // NOTE: this routine can be slowish without affecting much.
    //
    // XXX ??? might invoking this be more efficient via pthread_once?
    //
    // NOTE: we MUST use low-level routines, and not anything fancy
    // like CpuCycler, because CpuCycler REQUIRES that Apic::init()
    // already be complete!
    //
    void Apic::init()
    {
        // I don't care if I'm run twice on different CPUs, but
        // eventually I should not do anything.
        if( apic2cpu[0] == SOME_UNAVAILABLE_CPU_ID )
        {
            PASSERT( Apic::nCpus < 255U, "255 or more logical processors.  Please rewrite this code" );
            PASSERT( Apic::nCpus == util::getNCpus(), "libutil variable 'const uint32_t Apic::nCpus' seems to have been improperly initialized." );

            LOG(INFO, "Initializing APIC<->CPU translations for "
                    <<Apic::nCpus<<" cpus" );


            pid_t pid = getpid();
            uint32_t apicOrig = id(); // id() always works
            uint32_t cpuOrig;  // WARNING: cannot determine this properly yet
            cpu_set_t affinityOrig;
            util::getAffinity( pid, apicOrig, cpuOrig, affinityOrig );
            //                                ^^^^^^^
            //                                garbage value (for now) because
            //                                my translation table is not set up
            //                                               cpuOrig=   255
            LOG(INFO, "pid="<<pid<<" apicOrig="<<apicOrig<<" cpuOrig="<<cpuOrig);

            cpu_set_t affinity;
            CPU_ZERO( &affinity );
            // fill table (well, the first nCpus entries)
            for(uint32_t cpu=0U; cpu < Apic::nCpus; ++cpu)
            {
                MYTRACE("Setting Affinity for "<<cpu<<" ...");
                CPU_SET( cpu, &affinity );
                util::forceAffinity( pid, affinity );
                sched_yield(); // just in case -- I WANT to be absolutely sure
                // the reference table is correct.
                //
                // Note: libc/pthread docs DO NOT GUARANTEE this, as far as
                // I can see.  My own experiments SUGGEST that one on linux
                // the cpu migration is actually guaranteed after returning
                // from the schec_setaffinity call successfully, and that
                // the yield() in fact may not be necessary.  (yield() docs
                // do not indicate that if execution is currently on a
                // disallowed CPU that rescheduling is forced, so this
                // yielding paranoia PERHAPS does not even guarantee a cpu
                // switch) XXX
                {
                    uint32_t apicId = Apic::id();

                    MYTRACE(" ApicId="<<apicId<<endl);
                    PASSERT( apicId < 256U, "Unexpected APIC id (should be in [0,255])" );
                    PASSERT( apicId != SOME_UNAVAILABLE_CPU_ID,
                            "This motherboard has actually used APIC Id "
                            <<(uint32)SOME_UNAVAILABLE_CPU_ID
                            <<",  on a motherboard reporting "<<Apic::nCpus
                            <<" Cpus.\n\tCode for util::Apic should be re-evaluated."
                            //"\n\tDo you have a few hundred Cpus on this motherboard?"
                           );

                    apic2cpu[ apicId ] = (uchar)cpu;
                    LOG(INFO, "apic "<<apicId<<" --> cpu "<<cpu);
                }
                CPU_CLR( cpu, &affinity ); // affinity back to CPU_ZERO state
            }

            // get the REAL cpuOrig, now that we have the translation table set up
            cpuOrig = Apic::cpu( apicOrig );
            // For sure, put us back onto original CPU
            CPU_SET( cpuOrig, &affinity );
            util::forceAffinity( pid, affinity );
            // At this point, it is already permitted to call Apic:: functions such as:
            util::yieldIfNotOnApic( apicOrig );
            // THEN expand back to original CPU affinity.
            util::forceAffinity( pid, affinityOrig );

        }
        if( apic2cpu[ 0 ] == SOME_UNAVAILABLE_CPU_ID )
        {
            apic2cpu[0] = SOME_UNAVAILABLE_CPU_ID - 1U; // flag ourselves as having being run.
        }
    }


    void Apic::sampleAllTscs( TscMeasurement *rawData )
    {
        util::CpuCycler cc;
        // start sampling at whatever CPU we're currently running on.

        do // for each cpu, begining with current one
        {

            takeAReading( &rawData[cc.getCpu()] );

        }
        while( cc.nextCpu() );
        // exit running on cpu0 again
    }

    // quick estimate (not an example of high-performance timing)
    // All i need is very rough idea, within factor of 2 to 3 is
    // perfectly all right.
    static uint64_t ticks4systime()
    {
        uint64_t lo;
        //uint64_t hi;
        lo = std::numeric_limits<uint64_t>::max();
        //hi = 0ULL;
        struct timespec ts;
        {
            util::CpuCycler cc; // lock to "current" CPU.

            for( uint32 i=0U; i<10U; ++i )
            {
                util::cpuid_eax(0);
                uint64_t a = util::rdtsc();
                // usually 7000-20000 ticks
                clock_gettime( CLOCK_REALTIME, &ts );
                util::cpuid_eax(0);
                uint64_t b = util::rdtsc();
                lo = std::min( lo, b-a );
                //hi = std::max( hi, b-a );
            }
        }
        return lo;
    }


} // util
