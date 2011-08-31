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
 * vista.c
 *
 * DESCRIPTION
 *
 *   This file contains the main routines for the vista library. Vista
 *   lets the user map in persistent virtual memory, and perform 
 *   transactions on that memory in a very lightweight and simple 
 *   manner.
 *
 * PUBLIC FUNCTIONS
 *
 *   void vista_init(void);
 *   vista_segment* vista_map(char* filename, char *data_addr);
 *   int vista_unmap(vista_segment* sd);
 *   void vista_set_log(vista_segment* s);
 *   int vista_sigaction(int sig, struct sigaction *act,
 *				  struct sigaction *oact);
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
 *   (ifdef APP_MPROTECT) void mprotect(caddr_t addr, size_t len, int prot);
 * AUTHOR
 *   Dave Lowell 
 */

/**
 ** Includes
 **/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#define VISTA_DECLARE
#include "vista.h"

#if defined(__alpha)

#include <siginfo.h>

#elif defined(__FreeBSD__)

#include <vm/vm.h>
#include <vm/pmap.h>
#include <sys/signal.h>
#include <sys/param.h>
#include <sys/mount.h>

#endif

/**
 ** Global Variables
 **/

unrecov		unrecov_data;	/* Vista's global variables that need to NOT
					be recovered by Discount Checking */
unrecov		*unrecov_ptr = &unrecov_data;

/**
 ** Functions 
 **/

void vista_sigprot_sub(int, char *, struct sigcontext *, int);
void vista_sigother(int);
extern void vistagrams_init(void);
extern void vistagrams_map_init(vista_segment*);
extern void vistagrams_remap_init(vista_segment*);
extern void vistagrams_begin_transaction(vista_segment*, int);
extern void vistagrams_abort(vista_segment*, transaction_info*, int);
extern void vistagrams_commit(vista_segment*, transaction_info*);
extern int  vistagrams_log_empty(transaction_info*);
extern int  vistagrams_sigio_do_one_message(void);

#ifdef APP_MPROTECT

#if !defined(__FreeBSD__)
#       error "APP_MPROTECT can only be used on FreeBSD (for now)" 
#endif

#include <sys/syscall.h>

void merge_protect(int, int, char *, int);

char app_protect[NUMPAGES];

#endif

/* 
 * void vista_sigprot(int sig, struct siginfo *siPtr, struct sigcontext *scPtr)
 * void vista_sigprot(int sig, int code, struct sigcontext *sc, char* vaddr)
 *
 *   This function will be set up as the front-end signal handler for sigprot
 *   It isolates the machine dependencies and calls a machine-independent
 *   function to do the real work of handling the signal.
 *   There are separate versions for DEC Alphas running Digital Unix and PCs
 *   running FreeBSD.
 */

#if defined(__alpha)
void vista_sigprot(int sig, struct siginfo *siPtr, struct sigcontext *scPtr)
{
	vista_sigprot_sub(sig, siPtr->si_addr, scPtr,
				siPtr->si_code == SEGV_ACCERR);
}

#elif defined(__FreeBSD__)

void vista_sigprot(int sig, int code, struct sigcontext *sc_ptr, char* vaddr)
{
	int write_flag = code & (PGEX_W << 5);	/* code is only set to the right
						    value in our fixed version
						    of FreeBSD
						    (i386/i386/trap.c). */
#ifdef APP_MPROTECT
	int page_num = (unsigned long) VISTA_PTR_TO_PAGE(vaddr)/VISTA_PAGE;
	sigset_t set;

	vista_sigprot_sub(sig, vaddr, sc_ptr,
		write_flag && !(vista_protect[page_num] & PROT_WRITE) );

	/*
	 * Vista now thinks the page is writable, so mprotect
	 * according to the application's view.
	 */
	vista_protect[page_num] = PROT_READ | PROT_WRITE;
	syscall(SYS_mprotect, VISTA_PTR_TO_PAGE(vaddr), VISTA_PAGE,
	    app_protect[page_num]);

	/*
	 * Call application sighandler if application thinks the page is
	 * protected.  Don't call the application sighandler if the trap
	 * was generated in a Vista routine.
	 */
	if ( (write_flag && !(app_protect[page_num] & PROT_WRITE)) ||
		 (!write_flag && !(app_protect[page_num] & PROT_READ)) ) {
		if (lock_v) {
			fprintf(stderr, "warning: vista_sigprot saw lock_v\n");
			exit(1);
		}
		else {
			/*
			 * To avoid doing these sigprocmasks, I could simply
			 * add the signals to mask out when vista_init sets up
			 * sigaction for sigprot.  E.g. add sigio for
			 * TreadMarks.
			 */
			sigprocmask(SIG_BLOCK,
				&(siglist[VISTA_SIGPROT].sa_mask),
				&set);
			((void (*)()) *siglist[sig].sa_handler)
				(sig, code, sc_ptr, vaddr);

		}
	}

#else
	/*
	 * Can use write_flag=1 because read faults don't happen without
	 * mprotect.
	 */
	vista_sigprot_sub(sig, vaddr, sc_ptr, 1);
#endif /* APP_MPROTECT */
}

#endif /* __FreeBSD__ */

/*
 * void vista_sigprot_sub(int sig, char *addr, struct sigcontext *sc_ptr,
 *			int log_flag)
 *   This function does the real work of handling a sigprot.
 *   It will log the page if log_flag is set and the fault is in a segment
 *   with an active, implicit transaction.
 */
void vista_sigprot_sub(int sig, char *addr, struct sigcontext *sc_ptr,
	int log_flag)
{
	int     	i;
	int		tid;
	void    	*page_addr;
	int		page_num;
	vista_segment	*s;

	prot_lock();

	/*
	fprintf(stderr, "%s trap on addr 0x%lx, sig=%d\n",
		log_flag?"write":"read", addr, sig);
	*/

	/*
	 * Be careful what you do in this handler, especially before calling
	 * mprotect.  Make sure you don't write to memory (other than the
	 * stack).  E.g. for Discount Checking, heap is protected, so calling
	 * printf before mprotect will cause an infinite loop of traps.
	 * E.g. exit() changes the stream stuff on the heap.  _exit() is better.
	 */

	if (sig != VISTA_SIGPROT) {
		fprintf(stderr, "Internal error: wrong sig caught.\n");
		_exit(1);
	}

	/*
	 * Determine address of faulting page.
	 */
	page_addr = VISTA_PTR_TO_PAGE(addr);
	page_num = (unsigned long) page_addr/VISTA_PAGE;

	/* Is the faulting address in a Vista segment? */
	for (s = NULL, i = 0; s == NULL && i < num_segs; i++) {
		if (addr >= segments[i]->h->base &&
		    addr < segments[i]->h->base +
				VISTA_MAP_ADDR_INCREMENT) {
			s = segments[i];
		}
	}

	/* Is there an in-progress implicit transaction in this segment? */
	if (s != NULL) {
		for (tid = 0; tid < VISTA_MAX_TRANS &&
			(!s->t[tid].used ||
			 s->t[tid].transaction_committed != FALSE ||
			 s->t[tid].log_type != VISTA_IMPLICIT); tid++);
	}

	if (log_flag && s != NULL && tid < VISTA_MAX_TRANS) {
		/* Did user specify log segment? */
		if (log_seg != NULL) {
			s = log_seg;
			for (tid = 0;
			     tid < VISTA_MAX_TRANS && (!s->t[tid].used ||
				     s->t[tid].transaction_committed != FALSE ||
				     s->t[tid].log_type != VISTA_IMPLICIT);
			     tid++);
			if (tid == VISTA_MAX_TRANS) {
				fprintf(stderr, "Tried to log to segment without active transaction\n");
				_exit(1);
			}
		}

#ifdef APP_MPROTECT
		/*
		 * Make the page readable before trying to log it (only needed
		 * if the application has made the page !read).
		 */
		if ( ! (app_protect[page_num] & PROT_READ) ) {
			syscall(SYS_mprotect, page_addr, VISTA_PAGE, PROT_READ);
		}
#endif
		if (!log_range(s, page_addr, VISTA_PAGE, tid)) {
			vista_perror("sigprot handler log_range");
			_exit(1);
		}

	}
#if !defined(APP_MPROTECT)
	/*
	 * We unprotect the page regardless of whether or not we 
	 * log it to support lazy unprotect of segments on which
	 * implicit transactions are being done.
	 */
	mprotect(page_addr, VISTA_PAGE, PROT_READ | PROT_WRITE);
#endif
	prot_unlock();
}

/* 
 * void vista_init(void)
 *
 *   This function initializes vista and should be called exactly once
 *   before using any of the other vista routines.
 */
void vista_init(void)
{
	int	i;
	struct sigaction action;

	max_bytes_logged = 0;
	total_bytes_logged = 0;

	/*
	 * Initialize our structures that keep track of all open
	 * segments.
	 */
	for (i = 0; i < VISTA_MAX_SEGS; i++) 
		segments[i] = NULL;
	num_segs = 0;

	/*
	 * Set up the sigprot handler that will be the heart of 
	 * the implicit logging.
	 */
	action.sa_handler = (VISTA_HANDLER_T) vista_sigprot;
	sigemptyset(&action.sa_mask);
	action.sa_flags = SA_RESTART | SA_SIGINFO | SA_NODEFER;

	if (sigaction(VISTA_SIGPROT, &action, NULL) < 0) 
		perror("sigaction");
	
	vistagrams_init(); 

	/*
	 * Initialize global variables that aren't initialized elsewhere.
	 */
	lock_v = lock_p = defer_any = 0;
	log_seg = NULL;
	for (i = 1; i < NSIG; i++) {
		defer[i] = 0;
	}

#ifdef APP_MPROTECT
	/*
	 * Initialize Vista and application protection bits.
	 */
	memset(vista_protect, PROT_READ | PROT_WRITE, NUMPAGES);
	memset(app_protect, PROT_READ | PROT_WRITE, NUMPAGES);
#endif

	for (i = 1; i < NSIG; i++) {
		siglist[i].sa_handler = SIG_DFL;
		defer[i] = 0;
	}
}

/*
 * abort_transaction()
 *
 *   Used internally to abort transactions. It can also be used to clean
 *   up the logs when a transaction has been partially aborted (but 
 *   interrupted by, say, a crash).
 */
void abort_transaction(vista_segment* sd, transaction_info* t, int verbose)
{
	mem_chunk	*curr, *next;
	heap_op		*curr2, *next2;
	vista_segment	*sa;
	int		i;

	if (verbose)
		fprintf(stderr, "\nUndoing transaction...\n");
	/*
	 * We'll restore and free the log in two 
	 * steps so that a crash during restore
	 * wont be a problem.
	 */
	i = 0;
	for (curr = t->log_head; curr != NULL; curr = curr->next) {
		/*
		 * This won't cause traps because vista_abort_transaction
		 * has unprotected the segment.
		 */
		memcpy(curr->start, curr->data, curr->len);
		i++;
	}
	curr = t->log_head; 
	t->log_head = NULL;
	t->log_tail = NULL;
	for (; curr != NULL; curr = next) {
		next = curr->next;
		heap_free(sd->mh, curr->data, curr->len);
		heap_free(sd->mh, curr, sizeof(mem_chunk));
	}
	if (verbose)
		fprintf(stderr, "\tRestored %d ranges.\n", i);

	i = 0;
	if (t->op_head != NULL) {
		/*
		 * There were heap operations during the aborted 
		 * transaction, so ignore the frees and free the 
		 * mallocs.
		 */
		curr2 = t->op_head;
		t->op_head = NULL;
		t->op_tail = NULL;
		for (; curr2 != NULL; curr2 = next2) {
			next2 = curr2->next;
			sa = curr2->s; /* note the segment in which this operation was done */
			if (curr2->malloc_free == VISTA_MALLOC) {
				if (curr2->user_data) {
					alloc_info	*a;

					/*
					 * We check if this chunk of memory
					 * was already freed. It was freed
					 * already if the user freed it in
					 * the same transaction in which it
					 * was allocated.
					 */
					if ((a = vista_hash_find(sa->malloc_hash, curr2->p)) != NULL) {
						heap_free(sa->h, curr2->p, curr2->size);
						heap_free(sa->mh, a, sizeof(alloc_info));
						vista_hash_delete_entry(sa->malloc_hash, curr2->p);
					}
				}
				else
					heap_free(sa->mh, curr2->p, curr2->size);
				heap_free(sd->mh, curr2, sizeof(heap_op));
			}
			else {
				heap_free(sd->mh, curr2, sizeof(heap_op));
			}
			i++;
		}
	}
	if (verbose)
		fprintf(stderr, "\tUndid %d heap operations.\n", i);

	/*
	 * Now we back out of any vistagrams 
	 */
	vistagrams_abort(sd, t, verbose);

	t->transaction_committed = TRUE;
	t->used = FALSE;

	if (verbose) {
		fprintf(stderr, "Transaction aborted.\n\n", i);
	}
}


/*
 * commit_transaction()
 *
 * 	Function used internally to commit transactions.
 */
static void commit_transaction(vista_segment* s, transaction_info *t)
{
	mem_chunk	*curr, *next;
	heap_op		*curr2, *next2;
	vista_segment	*sa;

	/*
	 * Commit the transaction, and then process the various logs
	 */
	t->transaction_committed = TRUE;

	/*
	 * We handle the vistagrams stuff first since it can cause page
	 * faults when running DC. Those faults will cause pages to be
	 * copied to the undo log, so we need to handle all vistagram
	 * stuff before processing the undo log.
	 */
	vistagrams_commit(s, t); /* commit and send any vistagrams */

	curr = t->log_head;
	t->log_head = NULL;
	t->log_tail = NULL;
	for (; curr != NULL; curr = next) {
		next = curr->next;
		heap_free(s->mh, curr->data, curr->len);
		heap_free(s->mh, curr, sizeof(mem_chunk));
	}

	if (t->op_head != NULL) {
		/*
		 * There were heap operations during the transaction,
		 * so free the frees that we deferred and ignore the mallocs.
		 * We also have to go through and mark each allocation 
		 * committed. We do this first, in a separate idempotent
		 * step so we can safely leak the operation log if a crash
		 * occurs during the second loop.
		 */
		for (curr2 = t->op_head; curr2 != NULL; curr2 = curr2->next) {
			if (curr2->malloc_free == VISTA_MALLOC) {
				alloc_info *a;

				/*
				 * We need to mark any malloc committed that has
				 * not already been freed.
				 */
				a = vista_hash_find(curr2->s->malloc_hash, curr2->p);
				if (a != NULL)
					a->committed = TRUE;
			}
		}
		curr2 = t->op_head; 
		t->op_head = NULL;
		t->op_tail = NULL;
		for (; curr2 != NULL; curr2 = next2) {
			next2 = curr2->next;
			if (curr2->malloc_free == VISTA_MALLOC) {
				heap_free(s->mh, curr2, sizeof(heap_op));
			}
			else {
				sa = curr2->s; /* segment where chunk should be freed */
				/*
				 * Check to see if user or meta data
				 * needs to be freed. This is just so we
				 * can automatically make internal data
				 * get freed at commit time.
				 */
				if (curr2->user_data) {
					alloc_info *a = vista_hash_find(sa->malloc_hash, curr2->p);

					heap_free(sa->h, curr2->p, curr2->size);
					heap_free(sa->mh, a, sizeof(alloc_info));
					vista_hash_delete_entry(sa->malloc_hash, curr2->p);
				}
				else
					heap_free(sa->mh, curr2->p, curr2->size);
				heap_free(s->mh, curr2, sizeof(heap_op));
			}
		}
	}

	t->used = FALSE;
}


/*
 * void check_log(vista_segment* sd, transaction_info* t)
 *
 *   sd  -		descriptor for this segment
 *   t  -		pointer to struct with undo log and commitment state
 *
 *   Check to see if there is uncommitted data left in the segment. Such
 *   data exists in the segment when the log is non-empty, and the 
 *   transaction_committed flag is FALSE. When there is uncommitted data
 *   in the segment, we'll undo the operations by copying data out of the
 *   undo log. 
 */
void check_log(vista_segment* sd, transaction_info* t)
{
	if (t->transaction_committed == TRUE && t->log_head == NULL &&
	    t->op_head == NULL && vistagrams_log_empty(t)) {
		/* Reset these just in case... */
		t->log_tail = NULL;
		t->op_tail = NULL;
		t->used = FALSE;
	}
	else if (t->transaction_committed == TRUE) {
		fprintf(stderr, "Cleaning log...\n");
		/*
		 * We would only get into this situation if there
		 * was a crash immediately after committing. We'll 
		 * call commit_transaction() to free the log(s).
		 */
		commit_transaction(sd, t);
	}
	else if (t->transaction_committed == FALSE) {
		/*
		 * There's uncommitted data in the segment that we need
		 * to roll back to the most recently committed version.
		 */
		abort_transaction(sd, t, TRUE);
	}
	else if (t->transaction_committed == TENTATIVE) {
		/*
		 * Transaction is committed tentatively.  Do nothing here.
		 */
	}
}


/*
 * Check to see if existing vista file, or file to be created is in UFS.
 * Returns TRUE if file is in UFS, and FALSE otherwise. This distinction
 * matters only under FreeBSD. For alphas, this function always returns 
 * TRUE.
 */
int check_file(int fd)
{
#if defined(__FreeBSD__)
	struct statfs fs_info;

	if (fstatfs(fd, &fs_info) < 0) {
		fprintf(stderr, "fstatfs failed...\n");
		return FALSE;
	}

	if (fs_info.f_type == MOUNT_UFS) {
		return TRUE;
	}
	else {
		return FALSE;
	}
#else
	return TRUE;
#endif
}


/*
 * vista_segment* vista_map(char* filename, char* data_addr)
 *
 *   filename -		name of file to map in as persistent store
 *   data_addr -	user-specified address for data segment (NULL if
 *			user doesn't care)
 *
 *   Map in filename as a persistent store and return a segment descriptor
 *   so this new segment can be referenced by other vista routines. 
 *   The address at which the file is mapped is guaranteed to be the same
 *   as when the file was first mapped. If the file 'filename' does not
 *   exist, this function creates it and sets it up as a vista segment.
 */
vista_segment* vista_map(char* filename, char* data_addr)
{
	int	fd, i;
	char	*md, *addr;
	heap	*meta_heap, *data_heap, dummy;
	char	filename_meta[256];
	vista_segment *s;

	/*
	 * Check if we can open any more segments...
	 */
	if (num_segs == VISTA_MAX_SEGS) {
		vista_errno = VISTA_EMAXSEGS;
		return NULL;
	}

	vista_lock();

	/*
	 * Open the file and get a file descriptor for it. If the file
	 * doesn't exist, we'll create it.
	 */
	sprintf(filename_meta, "%s.meta", filename);
	fd = open(filename_meta, O_RDWR, 0777);
	if (fd < 0) {
		/*
		 * We next compute a unique address at which to map the meta
		 * heap for this segment.  The address is computed as some
		 * large distance (currently 64GB for Alpha, 256 MB for FreeBSD)
		 * beyond the end of the largest map address in use by the
		 * program at this point, and must be at least as large as the
		 * default mapping address.  A person using multiple map files
		 * should therefore map in all the existing map files they
		 * anticipate using before creating new ones. Otherwise the
		 * possibility of having two files map to the same address
		 * exists. Furthermore, files created by another program could
		 * quite possibly map to a location in use by a file created
		 * by this program. So, sharing is difficult.  We really need
		 * to assign globally unique mapping addresses.  We'll worry
		 * about this later.
		 */
		for (i=0, addr =
		    (char *) (VISTA_MAP_DEFAULT_ADDR-VISTA_MAP_ADDR_INCREMENT);
		    i<num_segs; i++) {
			if (segments[i]->mh->base > (char*)addr)
				addr = segments[i]->mh->base;
			if (segments[i]->h->base > (char*)addr)
				addr = segments[i]->h->base;
		}
		addr += VISTA_MAP_ADDR_INCREMENT;

		/*
		 * Create the metadata file
		 */
		fd = open(filename_meta, O_RDWR | O_CREAT, 0777);
		if (fd < 0) {
			vista_errno = VISTA_EFILE;
			vista_unlock();
			return NULL;
		}
		/*
		 * Make sure the file isn't in NFS if we're on FreeBSD
		 */
		if (!check_file(fd)) {
			vista_errno = VISTA_EFSTYPE;
			unlink(filename_meta);
			vista_unlock();
			return NULL;
		}
		/* 
		 * Set up meta heap which will hold internal data (log,
		 * keys, etc...) and metadata for the user heap.
		 */
		heap_init(&dummy, addr, addr, &dummy, fd); /* temp heap */
		md = morecore(&dummy, 1);
		if (md == NULL) {
			vista_errno = VISTA_EMAP;
			vista_unlock();
			return NULL;
		}
		meta_heap = (heap*) md;
		/* set up official meta heap */
		heap_init(meta_heap, dummy.base, dummy.limit, meta_heap, fd);

		/* 
		 * Set up user heap segment
		 */
		fd = open(filename, O_RDWR | O_CREAT, 0777);
		if (fd < 0) {
			vista_errno = VISTA_EFILE;
			vista_unlock();
			return NULL;
		}
		data_heap = &meta_heap[1];

		/*
		 * check for user-specified address for user heap
		 */
		if (data_addr == NULL) {
			addr += VISTA_MAP_ADDR_INCREMENT;
		} else {
			addr = data_addr;
		}
		/* set up official user heap */
		heap_init(data_heap, addr, addr, meta_heap, fd);

		/*
		 * Set up global segment structures and meta data segment
		 */
		s = (vista_segment*) heap_malloc(meta_heap, 
						 sizeof(vista_segment));
		if (s == NULL) {
			vista_errno = VISTA_ELOGALLOC;
			vista_unlock();
			return NULL;
		}
		s->h = data_heap;
		s->mh = meta_heap;
		meta_heap->key = s;
		for (i = 0; i < VISTA_MAX_TRANS; i++) {
			s->t[i].used = FALSE;
			s->t[i].log_head = NULL;
			s->t[i].log_tail = NULL;
			s->t[i].op_head = NULL;
			s->t[i].op_tail = NULL;
			s->t[i].transaction_committed = FALSE;
		}
		s->malloc_hash = (hash_table*) heap_malloc(s->mh, 
							  sizeof(hash_table));
		vista_hash_init(s->malloc_hash, HASH_ONE_WORD_KEYS, meta_heap);
		s->key_hash = (hash_table*) heap_malloc(s->mh, 
							sizeof(hash_table));
		vista_hash_init(s->key_hash, HASH_ONE_WORD_KEYS, meta_heap);

		/* Do the vistagram specific setup now */
		vistagrams_map_init(s);
	}
	else {
		char*	limit;

		/*
		 * Make sure the file isn't in NFS if we're on FreeBSD
		 */
		if (!check_file(fd)) {
			vista_errno = VISTA_EFSTYPE;
			vista_unlock();
			return NULL;
		}

		/*
		 * The file already existed so we'll assume that 
		 * it's a nice clean segment file that we created
		 * during a prior run. There is a risk that if the
		 * user maps in a file that existed, but that has not
		 * been set up as an VISTA heap, then we'll get some
		 * horrendous errors as we deal with the file's 
		 * contents. So, we'll do some rudimentary sanity
		 * checks on the file...
		 */
		lseek(fd, ((long)&(dummy.base))-((long)&dummy), SEEK_SET);
		if (read(fd, &addr, sizeof(void*)) < 0) {
			fprintf(stderr, "Cannot read map address.\n");
			vista_unlock();
			return NULL;
		}
		lseek(fd, ((long)&(dummy.limit))-((long)&dummy), SEEK_SET);
		if (read(fd, &limit, sizeof(void*)) < 0) {
			fprintf(stderr, "Cannot read map limit.\n");
			vista_unlock();
			return NULL;
		}
		md = mmap(addr, limit - addr, DEFAULT_PROT, MMAP_FLAGS, fd, 0);
		if (md == NULL) {
			vista_errno = VISTA_EMAP;
			vista_unlock();
			return NULL;
		}
		meta_heap = (heap*) md;

		/*
		 * Set up global segment structures
		 */
		s = (vista_segment*) meta_heap->key;

		/* Does the seg look ok? */
		if (s == NULL || s->mh != meta_heap) {
			vista_errno = VISTA_ESEGTRASHED;
			vista_unlock();
			return NULL;
		}

		s->mh->fd = fd;

		/* Set up the user's data segment */
		fd = open(filename, O_RDWR, 0777);
		if (fd < 0) {
			vista_errno = VISTA_EOPENLOG;
			vista_unlock();
			return NULL;
		}
		addr = s->h->base;
		limit = s->h->limit;
		s->h->fd = fd;
		md = mmap(addr, limit - addr, DEFAULT_PROT, MMAP_FLAGS, fd, 0);
		if (md == NULL) {
			vista_errno = VISTA_EMAP;
			vista_unlock();
			return NULL;
		}

		/* Do any vistagram specific stuff now */
		vistagrams_remap_init(s);

		/* 
		 * We'll make sure that we don't have any uncomitted
		 * data in the segment
		 */
		for (i = 0; i < VISTA_MAX_TRANS; i++) {
			if (s->t[i].used == TRUE) {
				check_log(s, &s->t[i]);
			}
		}
	}

	segments[num_segs++] = s;
	vista_unlock();
	return s;
}

/*
 * int vista_unmap(vista_segment* sd)
 *
 *   sd -		segment descriptor of segment to be unmapped
 *
 *   This routine unmaps the segment specified by sd. All the data in
 *   the segment is unreachable after this call completes. vista_unmap()
 *   returns TRUE if the unmap is successful and FALSE otherwise. 
 *   When FALSE is returned, vista_errno is set to indicate the error.
 */
int vista_unmap(vista_segment* sd)
{
	int	i, j;
	int	fd, fd_meta;

	vista_lock();

	/*
	 * Save the file descriptors while we can still access the segments.
	 */
	fd = sd->h->fd;
	fd_meta = sd->mh->fd;

	if (munmap(sd->h->base, sd->h->limit - sd->h->base) < 0) {
		vista_errno = VISTA_EUNMAPFAILED;
		vista_unlock();
		return FALSE;
	}
	else if (munmap(sd->mh->base, sd->mh->limit - sd->mh->base) < 0) {
		vista_errno = VISTA_EUNMAPFAILED;
		vista_unlock();
		return FALSE;
	}
	else {
		/*
		 * We need to update our array of segment addresses
		 * to remove this one. We want a contiguous list of 
		 * addrs, so we'll slide all addrs beyond this one
		 * down one position.
		 */
		for (i = 0; i < num_segs; i++)
			if (segments[i] == sd)
				break;
		if (i == num_segs) {
			fprintf(stderr, "Internal error: segment missing.\n");
			vista_unlock();
			return FALSE;
		}
		else if (i == VISTA_MAX_SEGS - 1)
			segments[i] = NULL;
		else {
			for (j = i; j < VISTA_MAX_SEGS - 1; j++) {
				segments[j] = segments[j+1];
				if (segments[j] == NULL)
					break;
			}
		}
	}

	num_segs--;

	/*
	 * Close the files
	 */
	close(fd);
	close(fd_meta);

	vista_unlock();
	return TRUE;
}

/*
 * int vista_set_log(vista_segment* s)
 *
 *   s -		segment to store all log records
 *
 *   This function directs Vista to store all implicitly logged undo records
 *   and heap operation log records in the specified segment. If segment is 
 *   NULL, use the normal algorithm of storing the undo records in the segment 
 *   containing the faulting address.
 */
void vista_set_log(vista_segment *s)
{
	log_seg = s;
}

/*
 * int vista_sigaction(int sig, struct sigaction *act, struct sigaction *oact)
 *
 * This function is used by applications to register signal handlers.
 * vista_sigother protects against vista concurrency issues by deferring
 * signals if the vista lock is held when the signal arrives.
 */

int vista_sigaction(int sig, struct sigaction *act, struct sigaction *oact)
{
	/*
	 * No need to lock, since it doesn't deal with shared data.
	 */
	struct sigaction action;

	if (sig < 1 || sig > NSIG) {
		fprintf(stderr, "vista_sigaction called with sig %d\n", sig);
		return(-1);
	}

	if (oact != NULL) {
		*oact = siglist[sig];
	}

	if (act != NULL) {
		/*
		 * Having both Vista and the application use sigprot is messy,
		 * so we keep it out of the base Vista distribution.
		 */

#if !defined(APP_MPROTECT)
		if (sig == VISTA_SIGPROT) {
			fprintf(stderr, "Sorry, Vista already uses sigprot\n");
			exit(1);
		}
#endif

		siglist[sig] = *act;

		if (sig == VISTA_SIGPROT) {
			/*
			 * Instead of re-installing Vista's signal handler with
			 * the new mask, it's simpler to let vista_sigprot
			 * set the new mask before calling the application's
			 * signal handler (otherwise DC has to re-install
			 * dc_sigprot).
			 */
		}
		else if (act->sa_handler == SIG_DFL && sig != SIGIO) {
			/*
			 * Re-install system default behavior
			 */
			sigaction(sig, act, NULL);
		}
		else {
			/*
			 * Re-direct signal to Vista's sigother handler, so
			 * it can deal with concurrency issues.  Vista
			 * handles SIGIO with vista_sigother just like other
			 * signals.  For SIGIO, this code's only purpose is to
			 * update the mask.  Note that Vista's signal handlers
			 * don't care what signals are masked out (i.e. they
			 * don't need any to be masked out, and they don't need
			 * any to be enabled).
			 */
			action = siglist[sig];
			action.sa_handler = (VISTA_HANDLER_T) vista_sigother;

			sigaction(sig, &action, NULL);
			/*
			 * When vista_sigother calls the application's signal
			 * handler, it should honor the mask requested during
			 * sigaction, including the implicit addition of the
			 * signal being handled.
			 */
			if ( ! (siglist[sig].sa_flags & SA_NODEFER) ) {
				sigaddset(&siglist[sig].sa_mask, sig);
			}
		}

		/*
		 * VISTA_SIGPROT should never be masked.
		 */
		sigdelset(&(siglist[sig].sa_mask), VISTA_SIGPROT);

	}
	return(0);
}

/*
 * This is Vista's signal handler for all signals other than sigprot.
 * It's also called by vista_unlock/prot_unlock if there were signals deferred
 * while the vista lock was held.
 */
void vista_sigother(int sig)
{
	sigset_t set;

	/*
	 * If no other vista routine is running when this signal came in,
	 * process the signal.  If a vista routine is running, the lock
	 * mechanism will defer the signal processing until vista's
	 * locks are released.
	 */
	if (vista_sig_lock(sig)) {
		if (sig == SIGIO) {
			while (vistagrams_sigio_do_one_message());
		}
		/*
		 * Call other user-specified signal handlers.
		 */
		if (siglist[sig].sa_handler != SIG_IGN &&
			siglist[sig].sa_handler != SIG_DFL) {

			(*siglist[sig].sa_handler)(sig);
		}
	}
}


/*
 * This function used by vista and vistagrams to make an entry in the heap
 * operation log for this transaction that some heap operation has been
 * performed during the transaction. If the 'user_data' parameter is
 * set to FALSE, this function can be used to delay the free or malloc
 * of metadata until commit. The vistagrams code makes occasional use
 * of this feature for automatically freeing internal stuff at commit
 * or abort. Segment 's' is the segment in which 'tid' is active. 
 * Segment 'sa' is the one in which this heap operation (malloc/free) was
 * performed. 's' and 'sa' can be the same.
 */
void log_heap_op(vista_segment *s, vista_segment *sa, int type, void* p, 
                 int size, int tid, int user_data)
{
	heap_op	*new;

	/*
	 * Check to make sure that we're logging in an active transaction. 
	 */
	if (s->t[tid].used==FALSE || s->t[tid].transaction_committed!=FALSE) {
		fprintf(stderr, "log_heap_op: tid not active transaction\n");
		return;
	}

	new = (heap_op*) heap_malloc(s->mh, sizeof(heap_op));
	if (new == NULL) {
		fprintf(stderr, "log_heap_op: internal malloc failed\n");
		return;
	}
	new->malloc_free = type;
	new->user_data = user_data;
	new->size = size;
	new->p = p;
	new->s = sa;  /* note which segment (heap) holds this allocation */
	new->next = NULL;
	if (s->t[tid].op_head == NULL) {
		s->t[tid].op_head = new;
		s->t[tid].op_tail = new;
	}
	else {
		s->t[tid].op_tail->next = new;
		s->t[tid].op_tail = new;
	}
}

/*
 * void* vista_malloc(vista_segment* s, int size, int tid)
 *
 *   s -		descriptor for segment from which to allocate storage
 *   size - 		number of bytes to allocate from segment sd
 *   tid - 		undo malloc if transaction tid aborts
 *
 *   Allocate size bytes from the region indicated by s. If successful,
 *   the function returns a pointer to the allocated storage, otherwise it
 *   returns NULL. If this malloc is being performed within transaction tid,
 *   it will be automatically freed if that transaction aborts. Passing
 *   VISTA_NO_TRANS for tid lets the malloc be done outside a transaction.
 *   If a log segment has been set up using vista_set_log(), 'tid' will be
 *   assumed to be a tid in the log segment, and all operation log records
 *   will be added there. 
 */
void* vista_malloc(vista_segment* s, int size, int tid)
{
	void 		*result;
	alloc_info 	*a;
	vista_segment 	*ls;

	vista_lock();

	/*
	 * Note that we're to log this operation in the log segment if one is
	 * set up, and in the user's segment if not.
	 */
	ls = (log_seg == NULL ? s : log_seg);

	result = heap_malloc(s->h, size);
	if (result != NULL) {
		/*
		 * We need to keep around some per malloc info, namely
		 * the size of the allocation, and if the malloc is
		 * being done within a transaction, whether that transaction
		 * committed.
		 */
		a = heap_malloc(s->mh, sizeof(alloc_info));
		if (a == NULL) {
			heap_free(s->h, result, size);
			return NULL;
		}
		a->size = size;
		a->committed = TRUE;

		vista_hash_insert(s->malloc_hash, result, (void*) a);
		if (tid != VISTA_NO_TRANS) {
			/*
			 * We are in a transaction so we need to log this
			 * malloc so it can be released on an abort. We
			 * also note that the surrounding tranasaction hasn't
			 * committed.
			 */
			a->committed = FALSE;
			log_heap_op(ls, s, VISTA_MALLOC, result, size, tid, TRUE);
		}
	}

	vista_unlock();
	return result;
}


/*
 * void* vista_realloc(vista_segment* s, void* p, int size)
 *
 *   s -		descriptor for region from which to allocate storage
 *   p - 		pointer to existing storage to expand or contract
 *   size - 		size in bytes of new region
 *
 *   Reallocates the memory pointed to by p to be size bytes long. The 
 *   function may have to move the region to accommodate the request. 
 *   If the request cannot be satisfied, NULL is returned and p is 
 *   unchanged. Otherwise, a pointer to the new region is returned.
 */
void* vista_realloc(vista_segment* s, void* p, int size)
{
	/* Currently this function is unimplemented */
	return NULL;
}


/*
 * void* vista_calloc(vista_segment* s, int nobjs, int objsize, int tid)
 *
 *   s -		descriptor for region from which to allocate storage
 *   nobjs -		number of objects for which to allocate cleared space
 *   objsize - 		size of each object in bytes
 *   tid - 		undo calloc if transaction tid aborts
 *
 *   Allocate sufficent storage from the region indicated by s to hold nobjs
 *   each of size objsize, and initialize the storage to all zeros. If 
 *   successful, the function returns a pointer to the allocated storage, 
 *   otherwise it returns NULL. Passing in VISTA_NO_TRANS for tid let's 
 *   this allocation be done outside a transaction.
 */
void* vista_calloc(vista_segment* s, int nobjs, int objsize, int tid)
{
	void	*result;
	int	size;

	size = nobjs * objsize;
	result = vista_malloc(s, size, tid);
	if (result != NULL)
		memset(result, 0, size);
	return result;
}


/*
 * void vista_free(vista_segment* s, void* p, int tid)
 *
 *   s -		descriptor for region from which storage was allocated
 *   p - 		pointer to storage to be freed
 *   tid - 		undo free if transaction tid aborts
 *
 *   Free the storage pointed to by p. The storage pointed to by p must have
 *   been allocated by a previous call to one of the vista allocation routines.
 *   If tid is not VISTA_NO_TRANS, this free will be undone if transaction
 *   tid aborts.
 */
void vista_free(vista_segment* s, void* p, int tid)
{
	alloc_info *a;
	vista_segment *ls;

	vista_lock();

	/*
	 * Note that we're to log this operation in the log segment if one is
	 * set up, and in the user's segment if not.
	 */
	ls = (log_seg == NULL ? s : log_seg);

	/*
	 * Look up the per malloc info for this chunk of memory
	 */
	a = vista_hash_find(s->malloc_hash, p);

	if (a != NULL) {
		if (tid != VISTA_NO_TRANS && a->committed) {
			/*
			 * We'll log this free and then hold off on doing
			 * the actual free until the transaction commits.
			 */
			log_heap_op(ls, s, VISTA_FREE, p, a->size, tid, TRUE);
		}
		else {
			heap_free(s->h, p, a->size);
			heap_free(s->mh, a, sizeof(alloc_info));
			vista_hash_delete_entry(s->malloc_hash, p);
		}
	}

	vista_unlock();
}

/*
 * int vista_set_key(vista_segment* s, int keynum, void* keyval)
 *
 *   s -		descriptor for region for which this key is relevant
 *   keynum -		indentifier for the key
 *   keyval -		key
 *
 *   This function lets the user associate a key value with a particular
 *   segment that can be retrieved by id later.  Returns TRUE or FALSE
 *   on success or failure respectively. 
 */
int vista_set_key(vista_segment* s, int keynum, void* keyval)
{
	vista_lock();
	vista_hash_insert(s->key_hash, (char*)keynum, keyval);
	vista_unlock();
	return TRUE;
}


/*
 * void* vista_get_key(vista_segment* s, int keynum)
 *
 *   s -		descriptor for region for which this key is relevant
 *   keynum -		indentifier for the key
 *
 *   This function lets the user retrieve a key that had previously been set
 *   as key keynum for segment s.
 */
void* vista_get_key(vista_segment* s, int keynum)
{
	void*	val;

	vista_lock();
	val = vista_hash_find(s->key_hash, (char*)keynum);
	vista_unlock();
	return val;
}


/*
 * int vista_begin_transaction(vista_segment* s, int type)
 *
 *   type - 		symbolic constant that specifies type of logging
 *   s - 		segment descriptor 
 *
 *   This routine begins a transaction on the persistent store. The type 
 *   parameter should be one of VISTA_IMPLICIT or VISTA_EXPLICIT and specifies
 *   whether implicit or explicit logging is used. A transaction identifier
 *   (tid) is returned that the user can use to commit, or abort this 
 *   transaction, as well as malloc or free within it.
 */
int vista_begin_transaction(vista_segment* s, int type)
{
	char		*base;	
	size_t		length;
	int		tid;

	if (type != VISTA_IMPLICIT && type != VISTA_EXPLICIT) {
		vista_errno = VISTA_EARGINVAL;
		return -1;
	}

	vista_lock();

	/*
	 * Obtain a tid and transaction_info struct for this transaction.
	 * We'll locate the first unused slot in the array...
	 */
	for (tid = 0; tid < VISTA_MAX_TRANS && s->t[tid].used; tid++);

	if (tid == VISTA_MAX_TRANS) {
		vista_errno = VISTA_EMAXTRANS;
		vista_unlock();
		return -1;
	}
	
	s->t[tid].transaction_committed = FALSE;
	s->t[tid].log_head = NULL;
	s->t[tid].log_tail = NULL;
	s->t[tid].op_head = NULL;
	s->t[tid].op_tail = NULL;
	s->t[tid].log_type = type;

	vistagrams_begin_transaction(s, tid);

	s->t[tid].used = TRUE;

	/*
	 * release vista lock before mprotect.  Otherwise mprotect will
	 * fault on the stack (for DC's data segment) and vista_sigprot will
	 * complain about lock being already held.  I could try just
	 * releasing the prot_lock before calling mprotect, but then
	 * when I re-get prot_lock and call vista_unlock, I might fault
	 * on the stack (on the way in to vista_unlock).
	 */
	vista_unlock();

	if (type == VISTA_IMPLICIT) {
		base = VISTA_PTR_TO_PAGE(s->h->base);
		length = s->h->limit - base;

		/* Make the entire segment read only */
#ifdef APP_MPROTECT
		memset(vista_protect + (unsigned long)base/VISTA_PAGE,
			PROT_READ, length/VISTA_PAGE);
		merge_protect((unsigned long)base/VISTA_PAGE, length/VISTA_PAGE,
			app_protect, PROT_READ);

#else
		if (mprotect(base, length, PROT_READ) < 0) 
			perror("mprotect");
#endif
	}

	bytes_logged = 0;
	return tid;
}

#ifdef APP_MPROTECT
/*
 * void merge_protect(int first_page, int num_pages, int prot)
 *
 * Update the system protection bits for each page p in the specified range
 * to be prot & prot_array[p].
 */
void
merge_protect(int first_page, int num_pages, char *prot_array, int prot)
{
	int i;
	int start;
	extern int Tmk_proc_id;

	start = first_page;
	while (start < first_page + num_pages) {
		/*
		 * Look for first page with different protection than start.
		 */
		for (i = start+1; i < first_page + num_pages &&
		   ((prot & prot_array[i]) == (prot & prot_array[start]));
		    i++);

		syscall(SYS_mprotect, (char *) (start*VISTA_PAGE),
			    (i-start)*VISTA_PAGE, prot & prot_array[start]);
		start = i;
	}
}

/* 
 * mprotect
 *
 *   This function intercepts libc's mprotect, so that application protection
 *   work with Vista's implicit transactions.
 */
int mprotect(caddr_t addr, size_t len, int prot)
{
	/*
	 * Get the vista lock so mprotect isn't interrupted by signals.
	 */
	vista_lock();

	/*
	 * Release the prot lock in case memset causes a trap on the app_protect
	 * array (which is logged under DC).
	 */
	prot_unlock();

	/*
	 * must set application protection bits before changing system
	 * protection bits.  Otherwise Vista's sighandler might be invoked
	 * without a correct picture of the application's protection bits.
	 */
	memset(app_protect + (unsigned long)addr/VISTA_PAGE, prot,
	    len/VISTA_PAGE);

	merge_protect((unsigned long)addr/VISTA_PAGE, len/VISTA_PAGE,
				vista_protect, prot);

	prot_lock();

	vista_unlock();

	return(0);
}
#endif /* APP_MPROTECT */

/*
 * int log_range()
 *
 *   Our general purpose range logger. This function makes an entry
 *   in the undo log for transaction tid so that if tid aborts, 
 *   all changes made to the 'len' bytes starting at address 'start'
 *   will be undone. In addition to handling user data undo logging,
 *   We use this function internally in some of the vistagrams code to 
 *   undo changes to metadata on abort.
 */
int log_range(vista_segment* s, void* start, int len, int tid)
{
	mem_chunk	*new;
	transaction_info *t;

	bytes_logged += len;
	total_bytes_logged += len;

	t = &s->t[tid];

	/* Log the data from this range */
	new = (mem_chunk*) heap_malloc(s->mh, sizeof(mem_chunk));
	if (new == NULL) {
		vista_errno = VISTA_ELOGALLOC;
		return FALSE;
	}
	new->data = (char*) heap_malloc(s->mh, len);
	if (new->data == NULL) {
		vista_errno = VISTA_ELOGALLOC;
		return FALSE;
	}
	memcpy(new->data, start, len);

	new->start = start;
	new->len = len;
	new->next = t->log_head;
	/* 
	 * Atomically add chunk to head of log. We put stuff in the
	 * log in LIFO order so that when we play back the log, stuff
	 * that was logged twice will be restored to the oldest version
	 * in the log. We consider a store of a single pointer atomic.
	 */
	if (t->log_head == NULL) {
		t->log_head = new;
		t->log_tail = new;
	}
	else 
		t->log_head = new;
	
	return TRUE;
}


/*
 * int vista_set_range(vista_segment* s, void* start, int len, int tid) 
 * 
 *   s - 		segment descriptor 
 *   start -		start of range in segment to be modified
 *   len -		the length of the range to be modified
 *   tid -		the transaction with which to group the modification
 *                      of this range.
 *
 *   This call informs vista that the region starting at start and extending
 *   for len bytes is about to be modified. All updates to the persistent 
 *   store within a transaction must be preceeded by a call to 
 *   vista_set_range(). It returns TRUE on success, and FALSE otherwise.
 *
 */
int vista_set_range(vista_segment* s, void* start, int len, int tid)
{
	int	retval;

	if (len == 0)
		return TRUE;

	prot_lock();

	if (!s->t[tid].used) {
		vista_errno = VISTA_EARGINVAL;
		retval = FALSE;
	}
	else {
		retval = log_range(s, start, len, tid);
	}
	
	prot_unlock();
	return retval;
}

/*
 * void vista_end_transaction(vista_segment* s, int tid)
 *
 *   s -		segment descriptor 
 *   tid -		transaction to commit
 *
 *   Atomically commits transaction tid on the persistent store. 
 */
void vista_end_transaction(vista_segment* s, int tid)
{
	vista_lock();

	if (!s->t[tid].used) {
		vista_errno = VISTA_EARGINVAL;
		vista_unlock();
		return;
	}

	/*
	 * Don't bother unprotecting the segment (we can do this lazily)
	 */

	/*
	 * Commit the transaction
	 */
	commit_transaction(s, &s->t[tid]);

	vista_unlock();

	if (bytes_logged > max_bytes_logged) {
	    max_bytes_logged = bytes_logged;
	}
}


/*
 * void vista_commit_tentative(vista_segment* s, int tid)
 *
 *   s -		segment descriptor 
 *   tid -		transaction to commit tentatively
 *
 *   Atomically marks that transaction 'tid' is tentatively committed.
 *   A tentatively committed transaction is permanent, however it
 *   can later be aborted explicitly by calling vista_abort_transaction
 *   with the same tid. If the process should crash, transaction 'tid'
 *   will not be aborted. The transaction can be finally committed
 *   by calling vista_end_transaction.
 */
void vista_commit_tentative(vista_segment* s, int tid)
{
	s->t[tid].transaction_committed = TENTATIVE;
}

/*
 * void vista_abort_transaction(vista_segment* s, int tid)
 *
 *   s -		segment descriptor
 *   tid -		transaction to abort
 *
 *   Transaction tid is aborted and the persistent store is restored to its
 *   state at the beginning of the transaction. Furthermore, all mallocs
 *   and frees from the persistent store are undone: the malloced data is
 *   automatically freed, and the freed data is reallocated.
 */
void vista_abort_transaction(vista_segment* s, int tid)
{
	char		*base;	
	size_t		length;

	vista_lock();

	if (s->t[tid].log_type == VISTA_IMPLICIT) {
		/*
		 * Unprotect the segment eagerly so playing back the undo logs
		 * doesn't cause more seg faults.
		 */
		base = VISTA_PTR_TO_PAGE(s->h->base);
		length = s->h->limit - base;

#ifdef APP_MPROTECT
		/*
		 * Don't want to take application seg faults.
		 */
		syscall(SYS_mprotect, base, length, PROT_READ|PROT_WRITE);

		abort_transaction(s, &s->t[tid], FALSE);

		/*
		 * Reinstantiate the application's protections.
		 * This can be done with the vista lock held (without worrying
		 * about mprotect trapping on the stack), because the user
		 * won't protect the stack.
		 */
		memset(vista_protect + (unsigned long)base/VISTA_PAGE,
			PROT_READ|PROT_WRITE, length/VISTA_PAGE);
		merge_protect((unsigned long)base/VISTA_PAGE, length/VISTA_PAGE,
			app_protect, PROT_READ | PROT_WRITE);
#else
		if (mprotect(base, length, PROT_READ|PROT_WRITE) < 0) 
			perror("mprotect");
		abort_transaction(s, &s->t[tid], FALSE);
#endif
	}
	else {
		abort_transaction(s, &s->t[tid], FALSE);
	}

	vista_unlock();
}


/*
 * void vista_perror(char* message)
 *
 *   message - 		message to prepend to error message
 *
 *   Prints out an error message to stderr based on the error currently
 *   stored in vista_errno. The system defined error message is prepended
 *   with the message provided as a parameter before printing.
 */
void vista_perror(char* message)
{
	fprintf(stderr, "%s: %s\n", message, vista_err_msgs[vista_errno]);
}
