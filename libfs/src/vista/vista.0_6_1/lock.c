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
 * lock.c
 *
 * DESCRIPTION
 *
 *   This file contains minimal support needed for vista's internal locking
 *   scheme. The only concurrency issue it attempts to address is the 
 *   concurrent sharing of vista data structures by vista routines and
 *   that could be reentered by user signals, or the vistagrams SIGIO signal.
 *
 *   The normal pattern of locking is:
 *	1) get the vista lock (lock_v)
 *	2) get the prot lock (lock_p)
 *	3) do "work"
 *	4) release the prot lock
 *	5) release the vista lock
 *
 *      Steps 1 and 2 are handled by vista_lock.  Steps 4 and 5 are handled by
 *      vista_unlock.
 *
 *   If "work" involves functions that cause page faults, then you should
 *   verify that it's ok (i.e. the page faults aren't happening when you
 *   are manipulating the undo log), then put a window around the function
 *   causing the page fault.  The window should release the prot lock, do
 *   the function, then get the prot lock back.
 *
 *   Signals are deferred when either lock_v or locks are held.
 *   Both prot_unlock and vista_unlock check for any deferred signals, but
 *   the signals can be processed only when neither lock is held.
 *
 *   For this to be used with threads, replace all the if(lock); lock with
 *   a call to pthread_mutex_lock, and replace all unlocks with
 *   pthread_mutex_unlock.
 *
 * PUBLIC FUNCTIONS
 *
 *   void vista_lock(void);		(MACRO)
 *   int  vista_sig_lock(int sig);
 *   void vista_unlock(void);
 *   void prot_lock(void);		(MACRO)
 *   void prot_unlock(void);
 *
 * AUTHOR
 *   Dave Lowell
 */

/**
 ** Includes
 **/

#include <stdio.h>
#include "vista.h"
#include "vistagrams.h"
#include "lock.h"
#include <string.h>


/**
 ** Functions
 **/

extern void vista_sigother(int sig);

/*
 * void check_deferred(void)
 *
 *   This function checks to see if there were any deferred signals to handle
 *   as a result of lock contention while the lock was held. It performs any
 *   such work it finds.
 */
void check_deferred(void)
{
	int		sig;
	sigset_t	set;

	while (!lock_v && !lock_p && defer_any) {
		/*
		 * Clear defer first, because vista_sigother may get lock
		 * and cause other signals to be deferred.  If we cleared
		 * defer after vista_sigother, these signals would get missed.
		 */
		defer_any = 0;
		for (sig = 1 ; sig < NSIG; sig++) {
			if (defer[sig]) {
				defer[sig] = 0;
				/*
				 * Honor the signal mask requested by the
				 * application.
				 */
				sigprocmask(SIG_BLOCK, &(siglist[sig].sa_mask),
						&set);
				vista_sigother(sig);
				sigprocmask(SIG_SETMASK, &set, NULL);
			}
		}
	}
}

/*
 * void prot_lock_sub(void)
 *
 *   Get the prot lock.
 */
#include <sys/syscall.h>
void prot_lock_sub(char *sfile, int line) 
{
	if (lock_p) {
		fprintf(stderr,
			"error: lock_p already held at file \"%s\", line %d\n",
			locked_in_file, locked_on_line);
		fprintf(stderr,
			"       relock attempt at file \"%s\", line %d\n",
			sfile, line);
		fprintf(stderr, "entering infinite loop\n");
		while(1);
	}

	lock_p = 1;

	/*
	 * Don't change the variables if prot_lock was called by vista_lock,
	 * because we actually want to know who called vista_lock.
	 */
	if (strcmp(sfile, "lock.c")) {
		locked_in_file = sfile;
		locked_on_line = line;
	}

}

/*
 * void prot_unlock(void)
 *
 *   Release the prot lock and check for any signals that were deferred.
 */
void prot_unlock(void) 
{
	lock_p = 0;
	check_deferred();
}

/*
 * void vista_lock_sub(void)
 *
 *   Lock the internal vista lock and prot lock.
 */
void vista_lock_sub(char *sfile, int line) 
{
	if (lock_v) {
		fprintf(stderr,
			"error: lock_v already held at file \"%s\", line %d\n",
			locked_in_file, locked_on_line);
		fprintf(stderr,
			"       relock attempt at file \"%s\", line %d\n",
			sfile, line);
		fprintf(stderr, "entering infinite loop\n");
		while(1);
	}

	lock_v = 1;

	prot_lock();

	locked_in_file = sfile;
	locked_on_line = line;
}


/*
 * int vista_sig_lock(int sig)
 *
 *   Check locks. If it is not already locked, return
 *   TRUE. Otherwise, set 'defer' flag so we remember we have deferred
 *   work to do when all locks get released.
 */
int vista_sig_lock(int sig) 
{
	if (lock_v || lock_p) {
		/* fprintf(stderr, "lock contention (signal %d), originally locked in \"%s\", line %d\n",
			sig, locked_in_file, locked_on_line); */
		defer_any = defer[sig] = 1;
		return FALSE;
	}
	else 
		return TRUE;
}


/*
 * void vista_unlock(void)
 *
 *   This function releases the vista and prot locks, then checks if there
 *   were any deferred signals.
 */
void vista_unlock(void) 
{
	prot_unlock();
	lock_v = 0;
	check_deferred();
}
