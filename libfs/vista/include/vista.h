/*
 * Copyright (c) 1997, 1998, 1999, 2000, David E. Lowell
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Vista library, version 0.6.1, September 2000
 */

/*
 * vista.h
 *
 * DESCRIPTION
 *
 *   This file contains the global declarations for the vista library. 
 *
 * PUBLIC FUNCTIONS
 *
 *   void vista_init(void);
 *   vista_segment* vista_map(char* filename, char *data_addr);
 *   int vista_unmap(vista_segment* sd);
 *   void vista_set_log(vista_segment* s);
 *
 *   int vista_set_key(vista_segment* s, int keynum, void* keyval);
 *   void* vista_get_key(vista_segment* s, int keynum);
 *
 *   void* vista_malloc(vista_segment* s, int size, int tid);
 *   void* vista_realloc(vista_segment* s, void* p, int size);
 *   void* vista_calloc(vista_segment* s, int nobjs, int objsize, int tid);
 *   void vista_free(vista_segment* s, void* p, int tid);
 *
 *   int vista_begin_transaction(vista_segment* s, int type);
 *   int vista_set_range(vista_segment* s, void* start, int len, int tid);
 *   void vista_end_transaction(vista_segment* s, int tid);
 *   void vista_abort_transaction(vista_segment *s, int tid);
 *   void vista_commit_tentative(vista_segment* s, int tid);
 *
 *   void vista_perror(char* message);
 *
 * PUBLIC DATA
 *
 *   int vista_errno;
 *
 * AUTHOR
 *   Dave Lowell 
 */

#ifndef VISTA_H
#define VISTA_H


/**
 ** Includes
 **/

#include <signal.h>
#include "heap.h"
#include "hash.h"
#include "vistagrams.h"
#include "lock.h"


/**
 ** Defines
 **/

/*
 * System specific macros
 */

#if defined(__alpha)

#define VISTA_MAP_DEFAULT_ADDR 0x20000000000L
#define VISTA_MAP_ADDR_INCREMENT 0x1000000000L  /* 64GB between maps */
#define VISTA_PAGE 8192
#define VISTA_SIGPROT SIGSEGV
#define VISTA_HANDLER_T void*

#elif defined(__FreeBSD__)

#define VISTA_MAP_DEFAULT_ADDR 0x50000000L
#define VISTA_MAP_ADDR_INCREMENT 0x10000000L  /* 256MB between maps */
#define VISTA_PAGE 4096
#define VISTA_SIGPROT SIGBUS
#define VISTA_HANDLER_T __sighandler_t*
#define TRUE 1
#define FALSE 0
#define SA_SIGINFO 0

#else
#       error "Sorry, Vista currently works only on Digital Unix and FreeBSD"
#endif

/*
 * Macros for everyone
 */

#define VISTA_MAX_SEGS 4
#define VISTA_LOG_DISP (VISTA_MAX_SEGS * VISTA_MAP_ADDR_INCREMENT)
#define VISTA_SEG_ADDR_MASK (~(VISTA_MAP_ADDR_INCREMENT - 1))
#define VISTA_MAX_TRANS 32
#define VISTA_PAGE_MASK (~(VISTA_PAGE - 1))
#define VISTA_PTR_TO_PAGE(ptr) ((void*) (((long)ptr) & VISTA_PAGE_MASK))
#define VISTA_NO_TRANS -1  /* tells vista to not do op log for heap ops */

#define VISTA_IMPLICIT 20
#define VISTA_EXPLICIT 21

#define VISTA_MALLOC 40
#define VISTA_FREE 41

#define TENTATIVE 2	/* more true than FALSE, but not fully TRUE */

/* Define our error codes */
#define VISTA_NOERROR 0
#define VISTA_ENOENT 1
#define VISTA_ESTAT 2
#define VISTA_EMAP 3
#define VISTA_EARGINVAL 4
#define VISTA_EMAXSEGS 5
#define VISTA_EFILE 6
#define VISTA_ENOTMAPPED 7
#define VISTA_EUNMAPFAILED 8
#define VISTA_ESEGTRASHED 9
#define VISTA_ESEGINVAL 10
#define VISTA_ECREATLOG 11
#define VISTA_EOPENLOG 12
#define VISTA_ELOGMAP 13
#define VISTA_ELOGALLOC 14
#define VISTA_ELOGTYPE 15
#define VISTA_EMAXTRANS 16
#define VISTA_EREQINUSE 17
#define VISTA_EBADHOST 18
#define VISTA_ERECV 19
#define VISTA_EPORTINUSE 20
#define VISTA_ESOCK 21
#define VISTA_EBADREQ 22
#define VISTA_ESEND 23
#define VISTA_ESELECT 24
#define VISTA_ESELECTRES 25
#define VISTA_ESELECTINT 26
#define VISTA_ESIGIO 27
#define VISTA_EMSGSIZE 28
#define VISTA_EFSTYPE 29

#ifdef VISTA_DECLARE

char* vista_err_msgs[] = {
	"no error", 				/* VISTA_NOERROR */
	"file not found",			/* VISTA_ENOENT */
	"cannot stat file",			/* VISTA_ESTAT */
	"cannot map file",			/* VISTA_EMAP */
	"invalid argument passed to routine",	/* VISTA_EARGINVAL */
	"maximum number of segments open",	/* VISTA_EMAXSEGS */
	"cannot open or create file",		/* VISTA_EFILE */
	"segment not previously mapped",	/* VISTA_ENOTMAPPED */
	"could not unmap segment",		/* VISTA_EUNMAPFAILED */
	"segment trashed",			/* VISTA_ESEGTRASHED */
	"invalid segment",			/* VISTA_ESEGINVAL */
	"cannot create log file",		/* VISTA_ECREATLOG */
	"cannot open log file",			/* VISTA_EOPENLOG */
	"cannot map log file",			/* VISTA_ELOGMAP */
	"cannot allocate log space",		/* VISTA_ELOGALLOC */
	"wrong log type",			/* VISTA_ELOGTYPE */
	"too many transactions",		/* VISTA_EMAXTRANS */
	"request in use",			/* VISTA_EREQINUSE */
	"invalid hostname",			/* VISTA_EBADHOST */
	"message receive failed",		/* VISTA_ERECV */
	"port in use",				/* VISTA_EPORTINUSE */
	"cannot open socket",			/* VISTA_ESOCK */
	"invalid request id",			/* VISTA_EBADREQ */
	"message send failed",			/* VISTA_ESEND */
	"socket select failed",			/* VISTA_ESELECT */
	"select returned weird result",		/* VISTA_ESELECTRES */
	"select was interrupted",		/* VISTA_ESELECTINT */
	"cannot configure sigio",		/* VISTA_ESIGIO */
	"message too long",			/* VISTA_EMSGSIZE */
	"vista file cannot be in NFS for FreeBSD" /* VISTA_EFSTYPE */
};

#else

extern char	*vista_err_msgs[];

#endif


/**
 ** Types
 **/

/*
 * This struct will hold a copy of a range of memory that is being
 * modified by a transaction. If the system crashes or the transaction
 * aborts, the memory can be restored to its original state by copying
 * the data from each of these chunks back to its original location.
 */
typedef struct mem_chunk_s {
	char*	start;		/* starting address of region modified */
	int	len;		/* length of modified region */
	char*	data;		/* copy of data from region */
	struct mem_chunk_s *next;  /* pointer to next chunk in list */
} mem_chunk;

/*
 * For each chunk of memory handed to the user by vista_malloc, we need 
 * to remember the size of that allocation, as well as whether or not the 
 * transaction surrounding that vista_malloc has committed. If it hasn't 
 * committed by the time the chunk is freed, the free doesn't need to be
 * deferred.
 */
typedef struct alloc_info_s {
	int	size;		/* size of malloc'ed chunk */
	int	committed;	/* did malloc's transaction commit? */
} alloc_info;

struct vista_segment;

/*
 * This struct describes a malloc or free operation performed during a 
 * transaction. The heap operation can be aborted or performed as needed
 * when the transaction commits or aborts.
 */
typedef struct heap_op_s {
	int	malloc_free;	/* was op malloc or free? */
	int	user_data;	/* is data in user or meta segment? */
	int	size;		/* size of malloc or free */
	void*	p;		/* pointer to malloced chunk or freed chunk */
	volatile struct vista_segment* s; /* segment where chunk was malloced */
	struct heap_op_s *next; /* next op in list */
} heap_op;

/*
 * The transaction info struct will hold the information about a single
 * segment's transactions. The system maintains an undo log pointed to
 * by log_head. For O(1) log appends, we also keep a pointer to the last
 * node in the log list in log_tail. The transaction on this segment is
 * not considered committed until the transaction_committed field is TRUE.
 */
typedef volatile struct transaction_info_s {
	int             used;       /* is this transaction struct in use? */
	mem_chunk       *log_head;  /* undo log head for this segment */
	mem_chunk       *log_tail;  /* undo log tail for this segment */
	heap_op         *op_head;   /* heap operations log head */
	heap_op         *op_tail;   /* heap operations log tail */
	int    		transaction_committed; /* true if t.a. has committed */
	int             log_type;   /* implicit or explicit? */
	vistagram_trans vg;         /* vistagram transaction info */
} transaction_info;


/*
 * The vista_segment struct holds global information about a segment.
 */
typedef volatile struct vista_segment {
	transaction_info        t[VISTA_MAX_TRANS]; /* Info on transactions */
	heap                    *h;             /* heap for use by user */
	heap                    *mh;            /* meta heap for use by vista */
	hash_table              *malloc_hash;   /* per malloc info */
	hash_table              *key_hash;      /* segment keys */
	message_info            minfo;          /* stuff needed for messages */
} vista_segment;


/*
 * This structure contains Vista's global data that it DOESN'T want Discount
 * Checking to log during normal operation, or restore during recovery.
 */
typedef struct unrecov_s {
	vista_segment	*segments[VISTA_MAX_SEGS];
	int		num_segs;
	int		vista_errno;

	volatile int	lock_v;		/* sigother handler must see lock */
	volatile int	lock_p;		/* sigother handler must see lock */
	char		*locked_in_file;/* src file that got lock_v or lock_p */
	int		locked_on_line; /* src line that got lock_v or lock_p */

	int		defer_any;
	int		defer[NSIG];
	struct sigaction siglist[NSIG];	/* user's signal handlers */

	int		retrans_sockets[VISTA_NPORTS]; /* socks receiving
							    retransmits */
	int		nsocks;		/* num sockets to watch */
	unsigned int	vista_localhost;/* encoded IP of localhost */
	int		cli_sock;	/* socket for client requests */

	vista_segment	*log_seg;	/* user-specified segment to log data
					    in */

	int		bytes_logged;	/* for performance measurements */
	long long 	total_bytes_logged;/* for performance measurements */
	int 		max_bytes_logged; /* for performance measurements */

#ifdef APP_MPROTECT
#define NUMPAGES (1024*1024)    /* 2^32 / 4096 */
	char vista_protect[NUMPAGES];	/* vista's protection bits */
#endif

} unrecov;

#define segments (unrecov_ptr->segments)
#define num_segs (unrecov_ptr->num_segs)
#define vista_errno (unrecov_ptr->vista_errno)
#define lock_v (unrecov_ptr->lock_v)
#define lock_p (unrecov_ptr->lock_p)
#define locked_in_file (unrecov_ptr->locked_in_file)
#define locked_on_line (unrecov_ptr->locked_on_line)
#define defer_any (unrecov_ptr->defer_any)
#define defer (unrecov_ptr->defer)
#define siglist (unrecov_ptr->siglist)
#define retrans_sockets (unrecov_ptr->retrans_sockets)
#define nsocks (unrecov_ptr->nsocks)
#define vista_localhost (unrecov_ptr->vista_localhost)
#define cli_sock (unrecov_ptr->cli_sock)
#define log_seg (unrecov_ptr->log_seg)
#define bytes_logged (unrecov_ptr->bytes_logged)
#define total_bytes_logged (unrecov_ptr->total_bytes_logged)
#define max_bytes_logged (unrecov_ptr->max_bytes_logged)

#ifdef APP_MPROTECT
#define vista_protect (unrecov_ptr->vista_protect)
#endif

/**
 ** Global Variables
 **/

extern unrecov * unrecov_ptr;

/**
 ** Functions 
 **/

extern void vista_init(void);
extern vista_segment* vista_map(char* filename, char *data_addr);
extern int vista_unmap(vista_segment* sd);
extern void vista_set_log(vista_segment* s);
extern int vista_set_key(vista_segment* s, int keynum, void* keyval);
extern void* vista_get_key(vista_segment* s, int keynum);
extern void* vista_malloc(vista_segment* s, int size, int tid);
extern void* vista_realloc(vista_segment* s, void* p, int size);
extern void* vista_calloc(vista_segment* s, int nobjs, int objsize, int tid);
extern void vista_free(vista_segment* s, void* p, int tid);
extern int vista_begin_transaction(vista_segment* s, int type);
extern int vista_set_range(vista_segment* s, void* start, int len, int tid);
extern void vista_end_transaction(vista_segment* s, int tid);
extern void vista_commit_tentative(vista_segment* s, int tid);
extern void vista_abort_transaction(vista_segment *s, int tid);
extern void vista_perror(char* message);
extern int vista_select(vista_segment* s, int* port_vector, int num_ports,
                        int ms_timeout);
extern int vista_send_request(vista_segment* s, char* host, int vg_port,
                              void* req, int req_len, int req_id, int tid);
extern int vista_receive_request(vista_segment* s, int vg_port, void* req,
                                 int* req_len, int* req_id, int tid);
extern int vista_send_response(vista_segment* s, void* response, int resp_len,
                               int req_id, int tid);
extern int vista_receive_response(vista_segment* s, int req_id, void* response,
                                  int *resp_len, int tid);
extern int vista_sigio_enable(vista_segment* s, int vg_port);

/*
 * We would like to be able to compile out support for 
 * vistagrams, since they're not compatible with DC.
 */

#endif

