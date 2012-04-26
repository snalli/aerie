#ifndef __STAMNOS_COMMON_PROFILER_H
#define __STAMNOS_COMMON_PROFILER_H

#include "common/hrtime.h"

#define PROFILER_PREAMBLE                          \
	hrtime_t      __start = hrtime_cycles();   \
	hrtime_t      __stop;                      \
	int           __startln = __LINE__; 

#define __PROFILER_SAMPLE        \
	hrtime_barrier();             \
	__stop = hrtime_cycles();    \
	printf("%s:%s:%d-%d: %llu\n", __FILE__, __FUNCTION__, __startln, __LINE__, __stop-__start); \
	__start = hrtime_cycles();   \
	__startln = __LINE__;

#define PROFILER_SAMPLE

#endif /* __STAMNOS_COMMON_PROFILER_H */
