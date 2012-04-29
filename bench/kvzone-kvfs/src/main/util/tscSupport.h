/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

/** \file tscSupport.h
 *  \brief miscellaneous support routines for timer code.
 */
#ifndef _TSCSUPPORT_H
#define _TSCSUPPORT_H

#include "tsc.h"

namespace util
{
#if 0 // obsolete
    /** APIC Id of current CPU (This reads CPU hardware register, and
     *  is not required to agree with kernel idea of "CPU number", although
     *  they may in fact be equivalent on many motherboards).
     *
     *  \obsolete use Apic::getId() instead, please.
     */ 
    uint32_t getApicId();
    TSCSUPPORT_INLINE __attribute__((always_inline)) uint32_t getApicId() //obsolete XXX
    {
        return Apic::id();
    }
#endif

    /** result = z - a, where z represents a time > a (assumed and unchecked) */
    void fwdtdiff(
            struct timespec &result,
            struct timespec const &z /*z>=a*/,
            struct timespec const &a );

    /** double version of fwdtdiff, z>=a precondition assumed, unchecked */
    double tdiff(
            struct timespec const &z /*z>=a*/,
            struct timespec const &a );

    /** quick conversion, handy for quick calcs or printing. (Not nec. fastest approach) */
    double tdouble( struct timespec const &t );

    /*
     * Generic CPUID function
     *
     * All CPUID calls flush instruction pipeline.  We are particularly
     * interested in determining upon which APIC (or even which processor
     * number from 0 to getNCpus()-1) we are currently executing.
     *
     * Note that the cpuid in general clobbers the ebx register, so it
     * is incompatible with -fPIC compilation for libutil.  For this,
     * we protect the cpuid call with push/pop of this register.  If you
     * use the /usr/include/linux/ version, compilation with -fPIC will
     * fail with "can't find a register in class BREG" message.
     *
     * Rewrite these inline assembler functions to support -fPIC
     * <B>as needed</B>.
     */
    //void cpuid(int32 op, int32 *eax, int32 *ebx, int32 *ecx, int32 *edx);
    
    /*
     * CPUID functions returning a single datum
     *
     */
    uint32 cpuid_eax(uint32 op);

    uint32 cpuid_ebx(uint32 op);

    //uint32 cpuid_ecx(uint32 op);

    //uint32 cpuid_edx(uint32 op);





    /////////////////////////// below: inlines




//#ifdef TSCSUPPORT_MAIN
//#define TSCSUPPORT_INLINE
//#else
#define TSCSUPPORT_INLINE inline __attribute__((always_inline))
//#endif

    /** r = z - a
     *
     * Assumes canonical timeval format:  tv_nsec within [0..999999999]
     *
     * \note assumes z > a (info gcc for a more generic routine)
     */
    TSCSUPPORT_INLINE void fwdtdiff( struct timespec &r, struct timespec const &z /*z>=a*/, struct timespec const &a )
    {
#define E9ULL 1000000000ULL
        r.tv_sec = z.tv_sec - a.tv_sec;
        // tv_nsec is long, so we can do signed cmp to 0
        // to check for carry.
#if 0 // info gcc says some systems have signed, this is maybe less good.
        if( (r.tv_nsec = z.tv_nsec - a.tv_nsec) < 0 )
        {
            // take 1 from seconds column
            r.tv_nsec += E9UL;
            --r.tv_sec;
        }
#else
        r.tv_nsec = z.tv_nsec - a.tv_nsec;
        if( z.tv_nsec < a.tv_nsec ) // fix underflow
        {
            r.tv_nsec += E9ULL;
            --r.tv_sec;
        }
#endif
#undef E9ULL
    }

    TSCSUPPORT_INLINE double tdiff( struct timespec const &z /*z>=a*/, struct timespec const &a )
    {
        return (z.tv_sec - a.tv_sec) + 1.e-9*((double)z.tv_nsec - a.tv_nsec);
    }

    TSCSUPPORT_INLINE double tdouble( struct timespec const &t )
    {
        return t.tv_sec + 1.e-9*t.tv_nsec;
    }

    ////////////////////////////// below: inlines ....

    // cpuid funcs, from asm/processor.h  cpuid_ebx(1)>>24 looks like
    // apicid from intel and amd.
#if 0 // too low-level to expose ?
    /*
     * Generic CPUID function
     */
    TSCSUPPORT_INLINE void cpuid(int32 op, int32 *eax, int32 *ebx, int32 *ecx, int32 *edx)
    {
        __asm__("cpuid"
                : "=a" (*eax),
                "=b" (*ebx),
                "=c" (*ecx),
                "=d" (*edx)
                : "0" (op));
    }
#endif

    /**
     * CPUID functions returning a single datum
     *
     * - modified to preserve ebx, for -fPIC compilation
     *
     * - made volatile for useful case where \c cpuid_eax(0) is used
     *   just to flush the instruction cache. (and we do not want the
     *   compiler to silently remove the cpuid function.
     */
#ifdef ARCH64
    TSCSUPPORT_INLINE uint32 cpuid_eax(uint32 op)
    {
        uint32 eax;
        __asm__ volatile (
                "pushq %%rbx\n\t"
                "cpuid\n\t"
                "popq %%rbx\n\t"
                : "=a" (eax)
                : "0" (op)
                : "cx", "dx");
        return eax;
    }
#else
    TSCSUPPORT_INLINE uint32 cpuid_eax(uint32 op)
    {
        uint32 eax;
        __asm__ volatile (
                "pushl %%ebx\n\t"
                "cpuid\n\t"
                "popl %%ebx\n\t"
                : "=a" (eax)
                : "0" (op)
                : "cx", "dx");
        return eax;
    }
#endif

#ifdef ARCH64
    TSCSUPPORT_INLINE uint32 cpuid_ebx(uint32 op)
    {
        uint32 ebx;
        // modified to preserve ebx
        __asm__ volatile (
                "pushq %%rbx\n\t"
                "cpuid\n\t"
                "movl %%ebx,%1\n\t"
                "popq %%rbx"
                : "=a" (ebx)
                : "0" (op)
                : "cx", "dx" );
        return ebx;
    }
#else
    TSCSUPPORT_INLINE uint32 cpuid_ebx(uint32 op)
    {
        uint32 ebx;
        // modified to preserve ebx
        __asm__ volatile (
                "pushl %%ebx\n\t"
                "cpuid\n\t"
                "movl %%ebx,%1\n\t"
                "popl %%ebx"
                : "=a" (ebx)
                : "0" (op)
                : "cx", "dx" );
        return ebx;
    }
#endif
#if 0
    TSCSUPPORT_INLINE uint32 cpuid_ecx(uint32 op)
    {
        uint32 eax, ecx;

        __asm__("cpuid"
                : "=a" (eax), "=c" (ecx)
                : "0" (op)
                : "bx", "dx" );
        return ecx;
    }
#endif
#if 0
    TSCSUPPORT_INLINE uint32 cpuid_edx(uint32 op)
    {
        uint32 eax, edx;

        __asm__("cpuid"
                : "=a" (eax), "=d" (edx)
                : "0" (op)
                : "bx", "cx");
        return edx;
    }
#endif

    // in general,
    //   ((Physical Package ID
    //      << (1 + ((int32)(log(2)(max(Logical_Per_Package-1, 1))))))
    //    || Logical ID)
    // is the initial apic id.
    // 
    // TBD: use /proc/cpuinfo to get at the number of physical CPUs,
    // investigate whether on HT we need to really have separate tsc
    // code per logical processor -- Intel uses per chip tsc clock,
    // so multiple cores should have same tsc.
    //

} // util

#endif // _TSCSUPPORT_H
