#ifndef __STAMNOS_SCM_MODEL_H
#define __STAMNOS_SCM_MODEL_H

#include <stdint.h>
#include <stddef.h>
#include "common/hrtime.h"
#include "common/util.h"

typedef uint64_t scm_word_t;

#include "scm/scm/asm.h"


#define STAMNOS_SCM_LATENCY_WRITE 150


/* Hardware Cache */

#ifdef __x86_64__
# define CACHELINE_SIZE     64
# define CACHELINE_SIZE_LOG 6
#else
# define CACHELINE_SIZE     32
# define CACHELINE_SIZE_LOG 5
#endif

#define CACHEBLOCK_ADDR(addr) ( (scm_word_t *) (((scm_word_t) (addr)) & ~(CACHELINE_SIZE - 1)) )
#define CACHEINDEX_ADDR(addr) ( (scm_word_t *) (((scm_word_t) (addr)) & (CACHELINE_SIZE - 1)) )




#define NS2CYCLE(__ns) ((__ns) * M_PCM_CPUFREQ / 1000)
#define CYCLE2NS(__cycles) ((__cycles) * 1000 / M_PCM_CPUFREQ)


#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)



/* Public types */


static inline
int rand_int(unsigned int *seed)
{
    *seed=*seed*196314165+907633515;
    return *seed;
}


static inline
void
emulate_latency_ns(int ns)
{
	hrtime_t cycles;
	hrtime_t start;
	hrtime_t stop;
	
	start = hrtime_cycles();
	cycles = HRTIME_NS2CYCLE(ns);

	do { 
		/* RDTSC doesn't necessarily wait for previous instructions to complete 
		 * so a serializing instruction is usually used to ensure previous 
		 * instructions have completed. However, in our case this is a desirable
		 * property since we want to overlap the latency we emulate with the
		 * actual latency of the emulated instruction. 
		 */
		stop = hrtime_cycles();
	} while (stop - start < cycles);
}


/**
 * Emulates the latency of a write to PCM. In reality, this method simply stores to an address and
 * spins to emulate a nonzero latency.
 *
 * \param set is the client's pcm storeset where outstanding stores are kept.
 * \param addr is the address being written.
 * \param val is the word written at address.
 */
template<typename T>
void
ScmStore(volatile T* addr, T val)
{
	*addr = val;
	ScmFlush(addr);
}


inline void 
ScmFence()
{
	asm_mfence(); 
	emulate_latency_ns(STAMNOS_SCM_LATENCY_WRITE);
}


/*
 * Flush the cacheline containing address addr.
 */
inline void
ScmFlush(volatile void *addr)
{
	asm_mfence(); 
	asm_clflush((volatile scm_word_t*) addr); 	
	emulate_latency_ns(STAMNOS_SCM_LATENCY_WRITE);
	asm_mfence(); 
}


static
void *
ScmLimitBandwidth(size_t n)
{
/*
 	scm_word_t*       val;
	size_t            size = n;
	int               extra_latency;
	double            throughput;

	if (size == 0) {
		return dst;
	}

	asm_mfence();
#ifdef SCM_EMULATE_LATENCY
	extra_latency = (int) size * (1-(float) (((float) SCM_BANDWIDTH_MB)/1000)/(((float) DRAM_BANDWIDTH_MB)/1000))/(((float)SCM_BANDWIDTH_MB)/1000);
	emulate_latency_ns(extra_latency);
	//asm_cpuid();
#endif
	stop = gethrtime();
	return dst;
*/
}



#endif // __STAMNOS_SCM_MODEL_H
