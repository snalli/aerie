/*
 * \file
 * \brief High-resolution nanosecond timer — portable via clock_gettime.
 */

#ifndef __STAMNOS_COMMON_HRTIME_H
#define __STAMNOS_COMMON_HRTIME_H

#include <time.h>

#ifndef _HRTIME_CPUFREQ
#define _HRTIME_CPUFREQ 2500 /* MHz — used only for cycle↔ns conversion macros */
#endif

#define HRTIME_NS2CYCLE(__ns)     ((__ns) * _HRTIME_CPUFREQ / 1000)
#define HRTIME_CYCLE2NS(__cycles) ((__cycles) * 1000 / _HRTIME_CPUFREQ)

typedef unsigned long long hrtime_t;

// Serialising barrier before a timer read
static inline void hrtime_barrier()
{
    __sync_synchronize();
}

// Returns nanoseconds since an arbitrary epoch (CLOCK_MONOTONIC)
static inline unsigned long long hrtime_cycles(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long) ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

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
