/**
 * \file
 *
 * PCM emulation over DRAM.
 *
 */
#ifndef _PCM_INTERNAL_H
#define _PCM_INTERNAL_H

#include <stdint.h>
#include <mmintrin.h>
#include <list.h>
#include <spinlock.h>


#define NS2CYCLE(__ns) ((__ns) * M_PCM_CPUFREQ / 1000)
#define CYCLE2NS(__cycles) ((__cycles) * 1000 / M_PCM_CPUFREQ)


#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)


/* Memory Pages */

#define PAGE_SIZE 4096

/* Returns the number of pages */
#define NUM_PAGES(size) ((((size) % PAGE_SIZE) == 0? 0 : 1) + (size)/PAGE_SIZE)

/* Returns the size at page granularity */
#define SIZEOF_PAGES(size) (NUM_PAGES((size)) * PAGE_SIZE)

/* Returns the size at page granularity */
#define PAGE_ALIGN(addr) (NUM_PAGES((addr)) * PAGE_SIZE)


/* Hardware Cache */

#ifdef __x86_64__
# define CACHELINE_SIZE     64
# define CACHELINE_SIZE_LOG 6
#else
# define CACHELINE_SIZE     32
# define CACHELINE_SIZE_LOG 5
#endif

#define BLOCK_ADDR(addr) ( (pcm_word_t *) (((pcm_word_t) (addr)) & ~(CACHELINE_SIZE - 1)) )
#define INDEX_ADDR(addr) ( (pcm_word_t *) (((pcm_word_t) (addr)) & (CACHELINE_SIZE - 1)) )


/* Public types */

typedef uintptr_t pcm_word_t;

typedef uint64_t pcm_hrtime_t;

typedef struct pcm_storeset_s pcm_storeset_t;
typedef struct cacheline_tbl_s cacheline_tbl_t;



/** 
 * Per client bookkeeping data structure that keeps several information
 * necessary to emulate latency and crashes for PCM on top of DRAM.
 */
struct pcm_storeset_s {
	uint32_t              id;
	uint32_t              state;
	unsigned int          rand_seed;
	PointerHash           *hashtbl;
	uint16_t              wcbuf_hashtbl[WCBUF_HASHTBL_SIZE];
	uint16_t              wcbuf_hashtbl_count;
	uint32_t              seqstream_len;
	cacheline_tbl_t       *cacheline_tbl;
	struct list_head      list;
	volatile unsigned int in_crash_emulation_code;
	uint64_t              seqstream_write_TS_array[8]; /* timestamp of writes */
	int                   seqstream_write_TS_index; 
};

/*
 * Locally defined global variables.
 */ 

/*
 * Externally defined global variables.
 */ 

extern unsigned int pcm_likelihood_store_blockwaits;  
extern volatile arch_spinlock_t ticket_lock;

/* 
 * Prototypes
 */

int pcm_storeset_create(pcm_storeset_t **setp);
void pcm_storeset_destroy(pcm_storeset_t *set);
pcm_storeset_t* pcm_storeset_get(void);
void pcm_storeset_put(void);
void pcm_wb_store_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val);
void pcm_wb_flush_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr);
void pcm_nt_store_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val);
void pcm_nt_flush_emulate_crash(pcm_storeset_t *set);


/*
 * Helper functions.
 */

static inline void asm_cpuid() {
	asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}

#if defined(__i386__)

static inline unsigned long long asm_rdtsc(void)
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

static inline unsigned long long asm_rdtscp(void)
{
		unsigned hi, lo;
	__asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi)::"ecx");
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );

}
#elif defined(__x86_64__)

static inline unsigned long long asm_rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

static inline unsigned long long asm_rdtscp(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtscp" : "=a"(lo), "=d"(hi)::"rcx");
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#else
#error "What architecture is this???"
#endif


static inline void asm_sse_write_block64(volatile pcm_word_t *addr, pcm_word_t *val)
{
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[0]): "r" (val[0]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[1]): "r" (val[1]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[2]): "r" (val[2]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[3]): "r" (val[3]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[4]): "r" (val[4]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[5]): "r" (val[5]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[6]): "r" (val[6]));
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*&addr[7]): "r" (val[7]));
}


static inline void asm_movnti(volatile pcm_word_t *addr, pcm_word_t val)
{
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*addr): "r" (val));
}


static inline void asm_clflush(volatile pcm_word_t *addr)
{
	__asm__ __volatile__ ("clflush %0" : : "m"(*addr));
}


static inline void asm_mfence(void)
{
	__asm__ __volatile__ ("mfence");
}


static inline void asm_sfence(void)
{
	__asm__ __volatile__ ("sfence");
}


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
	pcm_hrtime_t cycles;
	pcm_hrtime_t start;
	pcm_hrtime_t stop;
	
	start = asm_rdtsc();
	cycles = NS2CYCLE(ns);

	do { 
		/* RDTSC doesn't necessarily wait for previous instructions to complete 
		 * so a serializing instruction is usually used to ensure previous 
		 * instructions have completed. However, in our case this is a desirable
		 * property since we want to overlap the latency we emulate with the
		 * actual latency of the emulated instruction. 
		 */
		stop = asm_rdtsc();
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
}


/*
 * Flush the cacheline containing address addr.
 */
inline void
ScmFlush(volatile void *addr)
{
	asm_mfence(); 
	asm_clflush(addr); 	
	emulate_latency_ns(M_PCM_LATENCY_WRITE);
	asm_mfence(); 
}


static
void *
scm_memcpy_internal(scm_t *scm, void *dst, const void *src, size_t n)
{
	volatile uint8_t* saddr=((volatile uint8_t *) src);
	volatile uint8_t* daddr=dst;
	uint8_t           offset;
 	scm_word_t*       val;
	size_t            size = n;
	int               extra_latency;
	scm_hrtime_t      start;
	scm_hrtime_t      stop;
	scm_hrtime_t      duration;
	double            throughput;

	if (size == 0) {
		return dst;
	}

	start = gethrtime();
	/* First write the new data. */
	while(size >= sizeof(scm_word_t) * 8) {
		val = ((scm_word_t *) saddr);
		asm_sse_write_block64((uintptr_t *) daddr, val);
		saddr+=8*sizeof(scm_word_t);
		daddr+=8*sizeof(scm_word_t);
		size-=8*sizeof(scm_word_t);
	}
	if (size > 0) {
		/* Ugly hack: asm_sse_write_block64 requires a 64 bytes size value. We move
		 * back a few bytes to form a block of 64 bytes.
		 */ 
		offset = 64 - size;
		saddr-=offset;
		daddr-=offset;
		val = ((scm_word_t *) saddr);
		asm_sse_write_block64((uintptr_t *) daddr, val);

	}

	size = n;

	/* Now make sure data is flushed out */
	asm_mfence();
#ifdef SCM_EMULATE_LATENCY
	extra_latency = (int) size * (1-(float) (((float) SCM_BANDWIDTH_MB)/1000)/(((float) DRAM_BANDWIDTH_MB)/1000))/(((float)SCM_BANDWIDTH_MB)/1000);
	spin_lock(&(scm->bwlock));
	emulate_latency_ns(extra_latency);
	spin_unlock(&(scm->bwlock));
	//asm_cpuid();
#endif
	stop = gethrtime();
	duration = stop - start;
	throughput = 1000*((double) size) / ((double) CYCLE2NS(duration));
	stat_avg_double_add(&scm->stat.scm_bw, throughput);

	return dst;
}


void *
scm_memcpy(scm_t *scm, void *dst, const void *src, size_t n) 
{
	return scm_memcpy_internal(scm, dst, src, n);
}

#endif /* _PCM_INTERNAL_H */
