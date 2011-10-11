#ifndef _UT_BARRIER_H
#define _UT_BARRIER_H

#include <errno.h>
#include <pthread.h>

/* Portable barrier implementation based on POSIX threads */


typedef struct ut_barrier_s ut_barrier_t;

struct ut_barrier_sb_s {  
		pthread_cond_t  wait_cv;  /* cv for waiters at barrier    */  
		pthread_mutex_t wait_lk;  /* mutex for waiters at barrier */  
		int             runners;  /* number of running threads    */  
};  

struct ut_barrier_s { 
	int                    maxcnt;    /* maximum number of runners    */  
	struct ut_barrier_sb_s sb[2];  
    int                    sbi;       /* current sub-barrier          */  
};


static int  
ut_barrier_init(ut_barrier_t *bp, int count, int pshared)
{  
	int                  r;  
	int                  i;  
	pthread_mutexattr_t *m_attr = NULL;
	pthread_condattr_t  *cv_attr = NULL;
	pthread_mutexattr_t  psharedm_attr;
	pthread_condattr_t   psharedcv_attr;
  
	if (count < 1) { 
		return(EINVAL);  
	}	
  
	if (pshared) {
		pthread_mutexattr_init(&psharedm_attr);
		pthread_condattr_init(&psharedcv_attr);
		pthread_mutexattr_setpshared(&psharedm_attr, PTHREAD_PROCESS_SHARED);
		pthread_condattr_setpshared(&psharedcv_attr, PTHREAD_PROCESS_SHARED);
		m_attr = &psharedm_attr;
		cv_attr = &psharedcv_attr;
	}

	bp->maxcnt = count;  
	bp->sbi = 0;  
  
	for (i = 0; i < 2; ++i) {  
		bp->sb[i].runners = count;  
  
		if (r = pthread_mutex_init(&bp->sb[i].wait_lk, m_attr)) {  
			return(r);  
		}	
  
		if (r = pthread_cond_init(&bp->sb[i].wait_cv, cv_attr)) {  
			return(r);  
		}	
   }  
   return(0);  
}  
  

static int  
ut_barrier_wait(register ut_barrier_t *bp) 
{  
	register int sbi = bp->sbi;  

	pthread_mutex_lock(&bp->sb[sbi].wait_lk);  

	if (bp->sb[sbi].runners == 1) {    /* last thread to reach barrier */  
		if (bp->maxcnt != 1) {  
			/* reset runner count and switch sub-barriers */  
			bp->sb[sbi].runners = bp->maxcnt;  
			bp->sbi = (bp->sbi == 0) ? 1 : 0;  
  
			/* wake up the waiters */  
			pthread_cond_broadcast(&bp->sb[sbi].wait_cv);  
      	}  
    } else {  
		bp->sb[sbi].runners--;    /* one less runner */  
		while (bp->sb[sbi].runners != bp->maxcnt) { 
			pthread_cond_wait( &bp->sb[sbi].wait_cv, &bp->sb[sbi].wait_lk);  
		}	
	}  
  
	pthread_mutex_unlock(&bp->sb[sbi].wait_lk);  
  
    return(0);  
}  
  
  
static int  
ut_barrier_destroy(ut_barrier_t *bp) {  
	int r;  
	int i;  
  
	for (i=0; i < 2; ++ i) {  
		if (r = pthread_cond_destroy(&bp->sb[i].wait_cv)) {  
			return(r);  
		}	
  
		if (r = pthread_mutex_destroy( &bp->sb[i].wait_lk)) { 
			return(r);  
		}  
	}
	return (0);
}	

#endif
