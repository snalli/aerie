#include <linux/delay.h>
#include <linux/fs.h>
#include "internal.h"

typedef uint64_t scm_hrtime_t;

#define NS2CYCLE(__ns) ((__ns) * SCM_CPUFREQ / 1000)
#define SCM_CPUFREQ 2400LLU /* GHz */

static inline unsigned long long asm_rdtsc(void)
{
        unsigned long long int x;
        __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
        return x;
}

void emulate_latency_ns(int ns)
{
        scm_hrtime_t cycles;
        scm_hrtime_t start;
        scm_hrtime_t stop;

	//printk(KERN_ERR"in emulate latency for %d ns", ns);

        start = asm_rdtsc();
        cycles = NS2CYCLE(ns);

        do {
/* RDTSC doesn't necessarily wait for previous instructions to complete so 
 * a serializing instruction is usually used to ensure previous
 * instructions have completed. However, in our case this is a desirable
 * property since we want to overlap the latency we emulate with the
 * actual latency of the emulated instruction.
 */ 
                stop = asm_rdtsc();
        } while (stop - start < cycles);
}
                                             
int scm_simple_write_end(struct file *file, struct address_space *mapping, 
		loff_t pos, unsigned len, unsigned copied, 
		struct page *page, void *fsdata)
{
	emulate_latency_ns(SCM_LATENCY);
	return simple_write_end(file, mapping,	pos, len, copied, page, fsdata);

}
