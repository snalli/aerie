#ifndef _SCM_H
#define _SCM_H

#include "scmmodel/scm.h"

#define SCM_EMULATE_LATENCY
//#undef SCM_EMULATE_LATENCY

#define MAX_NCPUS 32

typedef struct {
	uint64_t n_per_cpu[MAX_NCPUS];
	double   avg_per_cpu[MAX_NCPUS];
} stat_avg_double_t;


typedef struct {
	uint64_t n_per_cpu[MAX_NCPUS];
} stat_aggr_uint64_t;


typedef struct {
	stat_avg_double_t  scm_bw;
	stat_aggr_uint64_t bytes_written;
	stat_aggr_uint64_t bytes_read;
} scm_stat_t;

	
struct scm_s {
	spinlock_t         lock;
	spinlock_t         bwlock; /* bandwidth model lock */
	uint32_t           count;
	volatile uint8_t   mode;
	scm_stat_t         stat;
};



/* 
 * Prototypes
 */

int scm_init(scm_t **);
void scm_fini(scm_t *);
void *scm_memcpy(scm_t *scm, void *dst, const void *src, size_t n);

void scm_stat_reset(scm_t *scm);
void scm_stat_print(scm_t *scm);

void stat_aggr_uint64_add(stat_aggr_uint64_t *stat, uint64_t val);

#endif /* _SCM_H */
