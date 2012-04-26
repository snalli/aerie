#ifndef __STAMNOS_COMMON_PROFILER_H
#define __STAMNOS_COMMON_PROFILER_H

#include "common/hrtime.h"

#define PROFILER_PREAMBLE                   \
	hrtime_t      start = hrtime_cycles();   \
	hrtime_t      stop;                      \
	int           startln = __LINE__; 

#define __PROFILER_SAMPLE        \
	hrtime_barrier();             \
	stop = hrtime_cycles();    \
	printf("%s:%s:%d-%d: %llu\n", __FILE__, __FUNCTION__, startln, __LINE__, stop-start); \
	start = hrtime_cycles();   \
	startln = __LINE__;

#define PROFILER_SAMPLE

#endif /* __STAMNOS_COMMON_PROFILER_H */
