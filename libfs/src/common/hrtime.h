/*
 * \file
 *
 * \brief High-resolution cycle counter — x86/64, AArch64, and portable fallback.
 *
 */

#ifndef __STAMNOS_COMMON_HRTIME_H
#define __STAMNOS_COMMON_HRTIME_H

#ifndef _HRTIME_CPUFREQ
#define _HRTIME_CPUFREQ 2500 /* MHz */
// insight : All CPu Frequencies aren't same !!!!!!
#endif

#define HRTIME_NS2CYCLE(__ns) ((__ns) * _HRTIME_CPUFREQ / 1000)
#define HRTIME_CYCLE2NS(__cycles) ((__cycles) * 1000 / _HRTIME_CPUFREQ)

typedef unsigned long long hrtime_t;

// ---------------------------------------------------------------------------
// Serialising barrier before cycle read
// ---------------------------------------------------------------------------
static inline void hrtime_barrier()
{
#if defined(__x86_64__) || defined(__i386__)
    asm volatile("cpuid" ::: "rax", "rbx", "rcx", "rdx");
#elif defined(__aarch64__)
    asm volatile("isb" ::: "memory");
#else
    __sync_synchronize();
#endif
}

// ---------------------------------------------------------------------------
// Cycle counter
// ---------------------------------------------------------------------------
#if defined(__i386__)

static inline unsigned long long hrtime_cycles(void)
{
    unsigned long long int x;
    __asm__ volatile(".byte 0x0f, 0x31" : "=A"(x));
    return x;
}

#elif defined(__x86_64__)

static inline unsigned long long hrtime_cycles(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
}

#elif defined(__aarch64__)

static inline unsigned long long hrtime_cycles(void)
{
    unsigned long long val;
    asm volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

#else

#include <time.h>
static inline unsigned long long hrtime_cycles(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long) ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

#endif

#define HRTIME_DEFINITIONS                                                                         \
    hrtime_t start __attribute__((unused)) = hrtime_cycles();                                      \
    hrtime_t stop __attribute__((unused));                                                         \
    int startln __attribute__((unused)) = __LINE__;

#define HRTIME_START                                                                               \
    start = hrtime_cycles();                                                                       \
    startln = __LINE__;

#define HRTIME_END stop = hrtime_cycles();

#define __HRTIME_SAMPLE                                                                            \
    stop = hrtime_cycles();                                                                        \
    printf("%s:%s:%d-%d: %llu\n", __FILE__, __FUNCTION__, startln, __LINE__, stop - start);        \
    start = hrtime_cycles();                                                                       \
    startln = __LINE__;

#define HRTIME_SAMPLE

#endif // __STAMNOS_COMMON_HRTIME_H
