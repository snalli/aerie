#include <linux/slab.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/preempt.h>
#include "scmdisk.h"
#include "scm.h"

								  
/* SCM write bandwidth */
int SCM_BANDWIDTH_MB = 1200;

/* SCM write latency */
int SCM_LATENCY_NS = 150;

/* DRAM system peak bandwidth */
int DRAM_BANDWIDTH_MB = 7000;

/* 
 * Statistics collection and processing
 */

void
stat_avg_double_reset(stat_avg_double_t *stat)
{
	int i;

	for (i=0; i<MAX_NCPUS; i++) {
		stat->n_per_cpu[i] = 0;
		stat->avg_per_cpu[i] =0;
	}
}

void
stat_aggr_uint64_reset(stat_aggr_uint64_t *stat)
{
	int i;

	for (i=0; i<MAX_NCPUS; i++) {
		stat->n_per_cpu[i] = 0;
	}
}


void
stat_avg_double_add(stat_avg_double_t *stat, double val)
{
	double   old_avg;
	double   new_avg;
	uint64_t n;
	int      id;

	//preempt_disable();
	id = smp_processor_id();
	old_avg = stat->avg_per_cpu[id];
	n = ++stat->n_per_cpu[id];
	new_avg = (((double) (n-1)) / ((double) n)) * old_avg + val/((double) n);
	stat->avg_per_cpu[id] = new_avg;
	//preempt_enable();
}

void
stat_avg_double_read(stat_avg_double_t *stat, double *val)
{
	double     total=0;
	double     avg = 0;
	uint64_t   n=0;
	int        i;
	
	for (i=0; i<MAX_NCPUS; i++) {
		if (stat->n_per_cpu[i] > 0) {
			n+=stat->n_per_cpu[i];
			total += stat->n_per_cpu[i]*stat->avg_per_cpu[i];
		}
	}
	if (n>0) {
		avg=total/n;
	}
	*val = avg;
}

void
stat_aggr_uint64_add(stat_aggr_uint64_t *stat, uint64_t val)
{
	int      id;

	//preempt_disable();
	id = smp_processor_id();
	stat->n_per_cpu[id] += val;
	//preempt_enable();
}

void
stat_aggr_uint64_read(stat_aggr_uint64_t *stat, uint64_t *val)
{
	uint64_t  total=0;
	int       i;
	
	for (i=0; i<MAX_NCPUS; i++) {
		total += stat->n_per_cpu[i];
	}
	*val = total;
}


void
scm_stat_reset(scm_t *scm)
{
	scm_stat_t *stat;

	printk(KERN_INFO "Reset statistics\n");
	stat = &scm->stat;
	stat_avg_double_reset(&stat->scm_bw);
	stat_aggr_uint64_reset(&stat->bytes_written);
	stat_aggr_uint64_reset(&stat->bytes_read);
}

void
scm_stat_print(scm_t *scm)
{
	scm_stat_t *stat = &scm->stat;
	double     scm_bw;
	uint64_t   bytes_read;
	uint64_t   bytes_written;

	stat_avg_double_read(&stat->scm_bw, &scm_bw);
	stat_aggr_uint64_read(&stat->bytes_read, &bytes_read);
	stat_aggr_uint64_read(&stat->bytes_written, &bytes_written);
	printk(KERN_INFO "SCM-DISK Statistics\n");
	printk(KERN_INFO "SCM_BW: %d\n", (int) scm_bw);
	printk(KERN_INFO "SCM_BYTES_READ: %llu\n", bytes_read);
	printk(KERN_INFO "SCM_BYTES_WRITTEN: %llu\n", bytes_written);
}


int
scm_init(scm_t **scmp)
{
	scm_t *scm;
	printk(KERN_INFO "scm_init:\n");

	if (!(scm = (scm_t *) kmalloc(sizeof(scm_t), GFP_KERNEL))) {
		return -ENOMEM;
	}

	scm->mode = MODE_NORMAL;
	spin_lock_init(&scm->lock);
	spin_lock_init(&scm->bwlock);
	printk(KERN_INFO "scm_init: DONE: scm=%p\n", scm);

	scm_stat_reset(scm);

	*scmp = scm;
	return 0;                 
}


void
scm_fini(scm_t *scm)
{
	printk("scm_fini: %p\n", scm);

	if (!scm) {
		return;
	}

	kfree(scm);
}


static inline void asm_sse_write_block64(uintptr_t *addr, scm_word_t *val)
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

static inline void asm_sse_write(volatile void *dst, uint64_t val)
{
	uint64_t* daddr = (uint64_t*) dst;
	__asm__ __volatile__ ("movnti %1, %0" : "=m"(*daddr): "r" (val));
}


static
void*
scm_memcpy_internal(scm_t* scm, void *dst, const void *src, size_t n)
{
	uintptr_t     saddr = (uintptr_t) src;
	uintptr_t     daddr = (uintptr_t) dst;
	uintptr_t     offset;
	uint64_t*     val;
	size_t        size = n;
	scm_hrtime_t  start;
	scm_hrtime_t  stop;
	scm_hrtime_t  duration;
	double        throughput;

	if (size == 0) {
		return dst;
	}

	// We need to align stream stores at cacheline boundaries
	// Start with a non-aligned cacheline write, then proceed with
	// a bunch of aligned writes, and then do a last non-aligned
	// cacheline for the remaining data.

	start = gethrtime();
	if (size >= CACHE_LINE_SIZE) {
		if ((offset = (uintptr_t) CACHE_LINE_OFFSET(daddr)) != 0) {
			size_t nlivebytes = (CACHE_LINE_SIZE - offset);
			while (nlivebytes >= sizeof(uint64_t)) {
				asm_sse_write((uint64_t *) daddr, *((uint64_t *) saddr));
				saddr+=sizeof(uint64_t);
				daddr+=sizeof(uint64_t);
				size-=sizeof(uint64_t);
				nlivebytes-=sizeof(uint64_t);
			}
		}
		while(size >= CACHE_LINE_SIZE) {
			val = ((uint64_t *) saddr);
			asm_sse_write_block64((uintptr_t *) daddr, val);
			saddr+=CACHE_LINE_SIZE;
			daddr+=CACHE_LINE_SIZE;
			size-=CACHE_LINE_SIZE;
		}
	}
	while (size >= sizeof(uint64_t)) {
		val = ((uint64_t *) saddr);
		asm_sse_write((uint64_t *) daddr, *((uint64_t *) saddr));
		saddr+=sizeof(uint64_t);
		daddr+=sizeof(uint64_t);
		size-=sizeof(uint64_t);
	}
	/* Now make sure data is flushed out */
	asm_mfence();

#ifdef SCMDISK_PERF_MODEL_ENABLE
	int extra_latency;
# ifdef SCMDISK_BANDWIDTH_MODEL_ENABLE
	extra_latency = (int) size * (1-(float) (((float) SCM_BANDWIDTH_MB)/1000)/(((float) DRAM_BANDWIDTH_MB)/1000))/(((float)SCM_BANDWIDTH_MB)/1000);
	spin_lock(&(scm->bwlock));
	emulate_latency_ns(extra_latency);
	spin_unlock(&(scm->bwlock));
# else
	extra_latency = SCM_LATENCY_NS;
# endif
	emulate_latency_ns(extra_latency);

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
	//return memcpy(dst, src, n);
}
