#ifndef _SCM_MODEL_H
#define _SCM_MODEL_H

#include <linux/slab.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/delay.h>

/* Static configuration */

/* Emulate latency */
#undef SCM_EMULATE_LATENCY
#define SCM_EMULATE_LATENCY 0x1

/* CPU frequency */
#define SCM_CPUFREQ 2500LLU /* GHz */

/* SCM write latency*/
#define SCM_LATENCY_WRITE 150 /* ns */

/* 
 * Stores may block wait to find space in the cache (write buffer is full and 
 * must wait to evict other cacheline from the cache) or to find an empty 
 * write-combining buffer.
 * 
 * For write-back, we emulate by using some probability to block-wait.
 * For write-combining, we keep track of the number of WC buffers being 
 * used. We conservatively assume that no implicit evictions happen.
 */
#define SCM_EMULATE_LATENCY_BLOCKING_STORES 0x1


/* Machine has the RDTSCP instruction. 
 * 
 * RDTSCP can be handy when measuring short intervals because it doesn't need
 * to serialize the processor first. Thus, it can be used to measure the actual 
 * latency of instructions such as CLFLUSH. This allows us to add an extra latency
 * to meet the desirable emulated latency instead of adding a fixed latency.
 */
#undef HAS_RDTSCP
//#define HAS_RDTSCP

/* The number of available write-combining buffers. */
#define WRITE_COMBINING_BUFFERS_NUM 8

/* The size of the WC-buffer hash table. Must be a power of 2. */
#define WCBUF_HASHTBL_SIZE WRITE_COMBINING_BUFFERS_NUM*4

/* The memory banking factor. Must be a power of 2. */
#define MEMORY_BANKING_FACTOR 8

/* Cache line size in log2 format . */
#define CACHELINE_SIZE_LOG 6 

/* Maximum page size */
#define PAGE_MAX_SIZE 8198


#define MODE_NORMAL     0x1

/* Definitions */

#define CACHELINE_SIZE (1 << CACHELINE_SIZE_LOG)
#define BLOCK_ADDR(addr) ( (scm_word_t *) (((scm_word_t) (addr)) & ~(CACHELINE_SIZE - 1)) )
#define INDEX_ADDR(addr) ( (scm_word_t *) (((scm_word_t) (addr)) & (CACHELINE_SIZE - 1)) )
#define NS2CYCLE(__ns) ((__ns) * SCM_CPUFREQ / 1000)
#define CYCLE2NS(__cycles) ((__cycles) * 1000 / SCM_CPUFREQ)

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define gethrtime       asm_rdtsc

/* Public types */

typedef uint64_t UINTPTR_T;
typedef uint64_t scm_word_t;
typedef uint64_t scm_hrtime_t;
typedef struct scm_s scm_t;
typedef struct cacheline_tbl_s cacheline_tbl_t;
typedef struct scm_storeset_s scm_storeset_t;

struct cacheline_tbl_s {
	struct list_head chain[PAGE_MAX_SIZE/CACHELINE_SIZE];
	unsigned int     size;
	unsigned int     count;
};

struct scm_storeset_s {
	scm_t                 *scm;
	uint32_t              id;
	uint32_t              state;
	unsigned int          rand_seed;
	struct kmem_cache     *cacheline_cache;
	cacheline_tbl_t       oldval_tbl;
	uint16_t              wcbuf_hashtbl[WCBUF_HASHTBL_SIZE];
	uint16_t              wcbuf_hashtbl_count;
	uint32_t              seqstream_len;
	struct list_head      scm_storeset_list;
};


/*
 * Helper functions.
 */

static inline void asm_cpuid(void) {
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


static inline void asm_movnti(volatile scm_word_t *addr, scm_word_t val)
{
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*addr): "r" (val));
}


static inline void asm_clflush(volatile scm_word_t *addr)
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
	scm_hrtime_t cycles;
	scm_hrtime_t start;
	scm_hrtime_t stop;
	
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
 * \brief Writes a masked word.
 * 
 * x86 memory model guarantees that single-byte stores are atomic, and
 * also that word-aligned word-size stores are atomic. If we wrote a
 * word-aligned word by reading its old value, masking-in the new value
 * and writing out the result, it would be not be always correct. To see
 * why consider another thread that writes a part of the same word but
 * its mask does not overlap with any other conrcurrent masked store.
 * Normally there is no race because we touch different parts. However
 * because we do word-size writes we introduce non-existing races. We
 * resolve this problem by writing each byte individually in the case
 * when we don't write the whole word. 
 * 
 * Note: Mask MUST not be zero.
 */
static inline 
void
write_aligned_masked(scm_word_t *addr, scm_word_t val, scm_word_t mask)
{
	UINTPTR_T a;
	int       i;
	int       trailing_0bytes;
	int       leading_0bytes;

	union convert_u {
		scm_word_t w;
		uint8_t    b[sizeof(scm_word_t)];
	} valu;

	/* Complete write? */
	if (mask == ((uint64_t) -1)) {
		*addr = val;
	} else {
		valu.w = val;
		a = (UINTPTR_T) addr;
		trailing_0bytes = __builtin_ctzll(mask) >> 3;
		leading_0bytes = __builtin_clzll(mask) >> 3;
		for (i = trailing_0bytes; i<8-leading_0bytes;i++) {
			*((uint8_t *) (a+i)) = valu.b[i];
		}
	}
}


/*
 * WRITE BACK CACHE MODE
 */


/**
 * Emulates the latency of a write to SCM. In reality, this method simply stores to an address and
 * spins to emulate a nonzero latency.
 *
 * \param set is the client's scm storeset where outstanding stores are kept.
 * \param addr is the address being written.
 * \param val is the word written at address.
 */
static inline
void
SCM_WB_STORE(scm_storeset_t *set, volatile scm_word_t *addr, scm_word_t val)
{
	*addr = val;
}


static inline
void
SCM_WB_STORE_ALIGNED_MASKED(scm_storeset_t *set, 
                            volatile scm_word_t *addr, 
                            scm_word_t val, 
                            scm_word_t mask)
{
	write_aligned_masked((scm_word_t *) addr, val, mask);
}



/*
 * Flush the cacheline containing address addr.
 */
static inline
void
SCM_WB_FLUSH(scm_storeset_t *set, volatile scm_word_t *addr)
{
#ifdef SCM_EMULATE_LATENCY
	{
# ifdef HAS_RDTSCP
		/* Measure the latency of a clflush and add an additional delay to
		 * meet the latency to write to SCM */
		scm_hrtime_t start;
		scm_hrtime_t stop;

		start = asm_rdtscp();
		asm_clflush(addr); 	
		stop = asm_rdtscp();
		emulate_latency_ns(SCM_LATENCY_WRITE - CYCLE2NS(stop-start));
# else
		asm_mfence();
		asm_clflush(addr); 	
		emulate_latency_ns(SCM_LATENCY_WRITE);
# endif		
		asm_mfence();
	}	

#else /* !SCM_EMULATE_LATENCY */ 
	asm_mfence();
	asm_clflush(addr); 	
	asm_mfence();
#endif /* !SCM_EMULATE_LATENCY */ 

}


/*
 * NON-TEMPORAL STREAM MODE
 *
 * Stores are non-cacheable but go through the write-combining buffers instead.
 */

static inline
void
SCM_NT_STORE(scm_storeset_t *set, volatile scm_word_t *addr, scm_word_t val)
{
#ifdef SCM_EMULATE_LATENCY
	uint16_t  i;
	uint16_t  index_addr;
	uint16_t  index_i;
	UINTPTR_T byte_addr;
	UINTPTR_T block_byte_addr;
#endif
	
	asm_movnti(addr, val);

#ifdef SCM_EMULATE_LATENCY
	byte_addr = (UINTPTR_T) addr;
	block_byte_addr = (UINTPTR_T) BLOCK_ADDR(byte_addr);
	index_addr = (uint16_t) ((block_byte_addr >> CACHELINE_SIZE_LOG)  & ((uint16_t) (-1)));

retry:
	if (set->wcbuf_hashtbl_count < WRITE_COMBINING_BUFFERS_NUM) {
		for (i=0; i<WCBUF_HASHTBL_SIZE; i++) {
			index_i = (index_addr + i) &  (WCBUF_HASHTBL_SIZE-1);
			if (set->wcbuf_hashtbl[index_i] == index_addr) {
				/* hit -- do nothing */
				break;
			} else if (set->wcbuf_hashtbl[index_i] == 0) {
				set->wcbuf_hashtbl[index_i] = index_addr;
				set->wcbuf_hashtbl_count++;
				break;
			}	
		}
	} else {
		memset(set->wcbuf_hashtbl, 0, WCBUF_HASHTBL_SIZE);
		emulate_latency_ns(SCM_LATENCY_WRITE * set->wcbuf_hashtbl_count);
		set->wcbuf_hashtbl_count = 0;
		goto retry;
	}

#endif
}


static inline
void
SCM_NT_FLUSH(scm_storeset_t *set)
{
	asm_sfence();
#ifdef SCM_EMULATE_LATENCY
	emulate_latency_ns(SCM_LATENCY_WRITE * set->wcbuf_hashtbl_count);
	memset(set->wcbuf_hashtbl, 0, WCBUF_HASHTBL_SIZE);
	set->wcbuf_hashtbl_count = 0;
#endif
}


/*
 * NON-TEMPORAL SEQUENTIAL STREAM MODE
 *
 * Used when we know that stream accesses are sequential so that we 
 * emulate bandwidth and hide some latency. For example, stores to the log
 * are sequential. 
 *
 */

 
static inline
void
SCM_SEQSTREAM_INIT(scm_storeset_t *set)
{
#ifdef SCM_EMULATE_LATENCY
	set->seqstream_len = 0;
#endif
}


static inline
void
SCM_SEQSTREAM_STORE(scm_storeset_t *set, volatile scm_word_t *addr, scm_word_t val)
{
	asm_movnti(addr, val);

#ifdef SCM_EMULATE_LATENCY
	set->seqstream_len = (set->seqstream_len + 1) & ((MEMORY_BANKING_FACTOR * 
	                                                  CACHELINE_SIZE / sizeof(scm_word_t) - 1));
	if (set->seqstream_len == 0) {
		emulate_latency_ns(SCM_LATENCY_WRITE);
	}
#endif
}


static inline
void
SCM_SEQSTREAM_FLUSH(scm_storeset_t *set)
{
	asm_sfence();
#ifdef SCM_EMULATE_LATENCY
	/* If we have pending stores then add latency */
	if (set->seqstream_len > 0) {
		emulate_latency_ns(SCM_LATENCY_WRITE);
		set->seqstream_len = 0;
	}
#endif
}


#endif /* !_SCM_MODEL_H */
