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
 * lock.h
 *
 * DESCRIPTION
 *
 *   This file contains minimal support needed for vista's internal locking
 *   scheme. The only concurrency issue it attempts to address is the 
 *   concurrent sharing of vista data structures by vista routines and
 *   the SIGIO handler needed for vistagrams.
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
 ** Macros
 **/

/* 
 * We use a macro to invoke vista_lock_sub so that we can get 
 * more detailed diagnotics if vista_lock fails.
 */
#define vista_lock() vista_lock_sub(__FILE__, __LINE__)
#define prot_lock() prot_lock_sub(__FILE__, __LINE__)

/**
 ** Functions
 **/

extern void vista_lock_sub(char *, int); 
extern int vista_sig_lock(int sig);
extern void vista_unlock(void);
extern void prot_lock_sub(char *, int); 
extern void prot_unlock(void); 
