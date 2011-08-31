/*
 * synth.h
 *
 *   Header information for synthetic benchmark exercising vista library.
 */

/**
 ** Include Files
 **/

#ifdef USE_VISTA
#include <vista.h>
#endif

#ifdef RVM_TRANS
#include <rvm.h>
#endif

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>


/**
 ** Defines
 **/

/* 
 * If USE_VISTA is defined, vista will be used for data support
 * VISTA_TRANS must also be defined for vista transactions to be
 * used. If the USE_VISTA is defined but not VISTA_TRANS, then 
 * no transactions support is used.
 *
 * If RVM_TRANS is defined, then RVM will provide both data and
 * transaction support.
 */

#define DB_SIZE 52428800   /* 50 Meg */

#ifdef USE_VISTA

#define LOOPS (200000 / (j<11?j+1:j*j))
#define WARMUP 50000

#define INIT(f) init_vista(f)
#define TRUNCATE_LOG() 
#define DATAFILE "data"
#define BENCH_LOOP for(i=0;i<times;i++)

#endif

#if defined(VISTA_TRANS) && defined(USE_VISTA)

/*
 * Vista transaction macros
 */

#define BEGIN_TRANS(tid) tid=vista_begin_transaction(s,VISTA_EXPLICIT)
#define SET_RANGE(tid, start, len) vista_set_range(s,start,len,tid)
#define END_TRANS(tid) vista_end_transaction(s,tid)

#elif defined(RVM_TRANS)

/*
 * RVM transaction macros
 */

#define LOOPS -1   /* run until truncation */
#define WARMUP 10000

#define INIT(f) init_rvm(f,RVM_RAW)
#define BEGIN_TRANS(tid) rvm_begin_transaction(tid,restore)
#define SET_RANGE(tid, start, len) rvm_set_range(tid,start,len)
#ifdef GROUP_COMMIT
#define END_TRANS(tid) rvm_end_transaction(tid,no_flush)
#else
#define END_TRANS(tid) rvm_end_transaction(tid,flush)
#endif
#define TRUNCATE_LOG() rvm_truncate()
#define BENCH_LOOP for(i=0,c=rvm_num_truncations;(times>0?i<times:c==rvm_num_truncations);i++)

#else

#define BEGIN_TRANS(tid) 
#define SET_RANGE(tid, start, len) 
#define END_TRANS(tid) 

#endif

#if defined(RVM_RAW) && (RVM_RAW == 1)

#define LOGFILE "/dev/rrz11e"
#define DATAFILE "/dev/rrz12c"

#elif defined(RVM_RAW) && (RVM_RAW == 0)

#define LOGFILE "rvmlog"
#define DATAFILE "rvmdata"

#endif

/**
 ** Global Variables
 **/

#ifdef USE_VISTA

vista_segment*		s;
int			tid;

#ifdef VISTA_TRANS
char*			version = "vista, trans";
#else
char*			version = "vista, notrans";
#endif

#endif

#ifdef RVM_TRANS

#if (RVM_RAW == 1)
#ifdef GROUP_COMMIT
char*			version = "RVM, trans, raw, group-commit";
#else
char*			version = "RVM, trans, raw";
#endif
#else
char*			version = "RVM, trans, cooked";
#endif
rvm_tid_t*		tid;

#endif

#ifdef GROUP_COMMIT

int	group_size;
#define GROUP_INC() g++
#define GROUP_FLUSH() if(g==group_size){g=0;rvm_flush();}

#else

#define GROUP_INC()
#define GROUP_FLUSH()

#endif


