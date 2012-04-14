#include <linux/slab.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/preempt.h>
#include "scm.h"

								  
/* SCM write bandwidth */
int SCM_BANDWIDTH_MB = 1200;

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
	printk(KERN_INFO "SCM_BYTES_READ: %lu\n", bytes_read);
	printk(KERN_INFO "SCM_BYTES_WRITTEN: %lu\n", bytes_written);
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
