#include "common/hrtime.h"
#include <sstream>

#define MEASURE_TIME_PREAMBLE    \
  struct timeval   time_stop;    \
  struct timeval   time_start;   \
  hrtime_t         hrtime_start; \
  hrtime_t         hrtime_stop; 


#define MEASURE_TIME_START            \
do {                                  \
  gettimeofday(&time_start, NULL);    \
  hrtime_start = hrtime_cycles();     \
} while(0);           


#define MEASURE_TIME_STOP             \
do {                                  \
  hrtime_stop = hrtime_cycles();      \
  gettimeofday(&time_stop, NULL);     \
} while(0);           


#define MEASURE_CYCLES_START          \
do {                                  \
  hrtime_start = hrtime_cycles();     \
} while(0);           


#define MEASURE_CYCLES_STOP           \
do {                                  \
  hrtime_stop = hrtime_cycles();      \
} while(0);           


#define ADD_MEASURE_TIME_DIFF_CYCLES(runtime)                        \
do {                                                                 \
  runtime += hrtime_stop - hrtime_start;                             \
} while (0);	




#define MEASURE_TIME_DIFF_USEC(runtime)                              \
do {                                                                 \
  runtime = 1000000 * (time_stop.tv_sec - time_start.tv_sec) +       \
                       time_stop.tv_usec - time_start.tv_usec;       \
} while (0);	


#define MEASURE_TIME_DIFF_CYCLES(runtime)                            \
do {                                                                 \
  runtime = hrtime_stop - hrtime_start;                              \
} while (0);	


static std::string 
measure_time_summary(int numops, unsigned long long runtime, hrtime_t runtime_cycles) 
{
	std::stringstream ss;
	hrtime_t runtime_hrtime = HRTIME_CYCLE2NS(runtime_cycles);
	
	ss << "runtime        " << runtime << " us (" << runtime/1000 << ")" << std::endl;
	if (numops>1) {
		ss << "avg_runtime    " << runtime/numops << " us (" << runtime/numops/1000 << ")" << std::endl;
	}
	
	ss << "runtime        " << runtime_cycles << " cycles";
	ss << "( " << runtime_hrtime << " ns)" << std::endl;
	if (numops>1) {
		ss << "avg_runtime    " << runtime_cycles/numops << " cycles";
		ss << "( " << runtime_hrtime/numops << " ns)" << std::endl;
	}
	return ss.str();
}
