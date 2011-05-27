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
 * heap.c 
 *
 * This module contains the code for a fast heap allocator that can take 
 * its heap management structure as an argument, letting the user keep
 * the heap struct anywhere the user likes. This heap system gets its 
 * memory from an mmapped file that is handed to its initialization routine. 
 * This code is poorly documented. 
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "heap.h"

/*
 * Overview of this heap management system
 *
 *	This heap is designed to be fast, small, and to let heap metadata
 * 	live anywhere the user likes. The memory it provides the user comes
 *	from a file mapped into the process' address space with mmap().
 *	This heap automatically extends the end of the file (and heap) as
 *	user needs grow. It makes no attempt to shorten the file as user
 *	needs shrink however. It also never unmaps any portion of the heap
 *	to return memory to the system.
 *	
 *	All memory requests are quantized to the nearest power of two size.
 *	The smallest allocatable size is eight bytes. For example, a request 
 *	for 150k results in 256k being returned to the user. All allocations 
 *	are at least quadword (8 bytes) aligned. Those allocations that are 
 *	8k or more in size are page aligned. 
 *
 *	This heap's routines are extremely fast, at a cost of being somewhat
 *	extravagant in its space usage. This heap does not keep track of 
 *	the size of each allocation as most heaps do (for example, by 
 *	storing the size of the allocation just before the start of the 
 *	returned chunk of memory). Therefore the size of each allocation 
 *	needs to be passed to the heap_free() routine when the allocation
 *	is returned to the heap. Although this is a little more work than
 * 	one usually has to do with a heap, the size of each allocation is 
 *	usually known statically. We impose this constraint because keeping
 *	the size of each allocation with the users chunk of memory 
 *	essentially puts heap metadata in a location in which it could be
 *	readily corrupted by the user. The alternative might be to keep 
 *	some sort of hash table for this information, but that might add
 *	significantly to the overhead of this extremely lightweight package.
 */

/*
 * heap_init()
 *
 *	h -	pointer to heap structure that will manage this heap
 *	base -	pointer to base of mapped region where heap will live
 *	limit -	pointer to end of mapped region
 *	allocator - heap from which this heap's internal data will be 
 *                  allocated. Can be the same as h.
 *	fd -	file descriptor for mmapped file for this region
 *
 *	This function initializes a heap management structure so that future
 *	mallocs and frees can be made with this heap. It returns nothing. 
 *	'limit' can point to the same location as 'base', indicating that
 *	no portion of the file has yet been mapped. It will be mapped 
 *	automatically the first time memory is allocated from this heap.
 *	Alternately, 'base' could point somewhere into a file already mapped.
 *	In this case, 'limit' should point to the end of the mapped region.
 */
void* heap_init(heap* h, void* base, void* limit, heap* allocator, int fd)
{
	int	i;

	for (i = 0; i < NBUCKETS; i++)
		h->bucketlists[i] = NULL;
	h->base = base;
	h->limit = limit;
	h->fd = fd;
	h->key = NULL;
	h->nlist = NULL;
	h->allocator = allocator;
}

/*
 * log2()
 *
 *	Compute the integer log base two of 'val'.
 */
static int log2(int val)
{
	int	x, i;
	for (x = 1, i = 0; val > x; x <<= 1, i++);
	return i;
}

/*
 * pow2()
 *
 *	Compute 2 to the power 'pow'.
 */
static int pow2(int pow)
{
	return (1 << pow);
}

/*
 * morecore() - add memory to this heap
 *
 *	h -	heap to which to add memory from the mapped file
 *	pages -	number of pages to add to the heap
 *
 *	This function adds 'pages' pages of memory to the heap 'h'. It 
 *	provides this memory by extending the file that backs this heap
 *	an appropriate amount, and then mapping the new portion. It returns
 *	a pointer to the new pages, or NULL on failure.
 */
void* morecore(heap* h, int pages)
{
	long	len, offset;
	void*	result;
	struct	stat stat_buf;
	char	buf = 0;

	offset = h->limit - h->base;
	len = pages * EXTENDSIZE;

	/* extend file */

	if (fstat(h->fd, &stat_buf) ) {
		perror("fstat");
		return NULL;
	}

	if (offset + len > stat_buf.st_size) {
		lseek(h->fd, offset + len - 1, SEEK_SET);
		if (write(h->fd, &buf, 1) < 0) {
			perror("write()");
			return NULL;
		}
	}

	/* map new portion */
	result = mmap(h->limit, len, DEFAULT_PROT, MMAP_FLAGS, h->fd, offset);
	if (result == (caddr_t)-1) 
		result =  NULL;
	else 
		h->limit += len;

	return result;
}


/*
 * nalloc() - 	allocate a nugget from this heap
 *
 *	h -	heap from which to grab a nugget
 *
 *	This function allocates and returns one nugget from heap 'h'.
 *
 *	The free lists for this heap consist of linked lists of 'nuggets'.
 *	Each nugget (node) in the linked list points to one free chunk of
 * 	memory in the heap that could be returned to the user during a 
 *	subsequent heap_malloc() call. The heap also maintains a free list
 *	of nuggets that can be allocated and freed internally as needed
 *	to maintain the free lists. The memory for these nuggets comes
 *	from this heap's 'allocator' which is specified when the heap is
 *	initialized. The allocator for this heap is another heap from
 *	which all internal data (the nuggets) will be allocated. A heap
 *	can be the allocator for itself. That just means that nuggets will
 *	be interspersed with user data.	Should a nugget be requested with 
 *	none waiting on the free list, the allocator's mapped region will
 *	automatically be extended to accomodate the new nugget(s).
 */
static nugget* nalloc(heap* h)
{
	nugget*	result;

	if (h->nlist != NULL) {
		/* Get nugget off the free list */
		result = h->nlist;
		h->nlist = result->next;
	}
	else {
		/* 
		 * Extend allocator by one page, and break it up
		 * new page into nuggets. 
		 */
		nugget	*new, *mem;
		int	i, chunks;

		mem = (nugget*) morecore(h->allocator, 1);
		if (mem == NULL) {
			fprintf(stderr, "nalloc: morecore failed.\n");
			return NULL;
		}
		chunks = EXTENDSIZE / sizeof(nugget);
		for (i = 0; i < chunks; i++) {
			new = &mem[i];
			new->addr = NULL;
			if (i == chunks - 1)
				new->next = NULL;
			else 
				new->next = &mem[i+1];
		}
		h->nlist = mem;
		
		result = h->nlist;
		h->nlist = result->next;
	}

	return result;
}

/*
 * nfree() 
 *
 *	Return nugget pointed to by 'n' to the nugget free list for 'h'.
 */
static void nfree(heap *h, nugget *n)
{
	n->next = h->nlist;
	h->nlist = n;
}


/*
 * heap_malloc() 
 *
 *	Allocated at least 'size' bytes from heap 'h'.
 */
void* heap_malloc(heap* h, int size)
{
	int	bucket;
	nugget*	n;
	void*	result;

	if (size == 0)
		return NULL;
	
	bucket = log2(size);
	bucket = (bucket < BUCKET_MIN ? BUCKET_MIN : bucket);
	n = h->bucketlists[bucket];
	if (n != NULL) {
		h->bucketlists[bucket] = n->next;
		result =  n->addr;
		nfree(h, n);
	}
	else {
		int	bsize, space, chunks, i;
		nugget	*last_nugget, *newn;
		char*	mem;

		/* We need more storage */
		if (size <= EXTENDSIZE)
			space = EXTENDSIZE;
		else
			space = pow2(bucket);
		mem = morecore(h, space / EXTENDSIZE);
		if (mem == NULL)
			return NULL;

		bsize = pow2(bucket);
		chunks = space / bsize;

		newn = nalloc(h);
		if (newn == NULL)
			return NULL;
		newn->addr = mem;
		newn->next = NULL;
		h->bucketlists[bucket] = newn;

		last_nugget = newn;
		for (i = 1; i < chunks; i++) {
			newn = nalloc(h);
			if (newn == NULL)
				break;
			newn->addr = &mem[i * bsize];
			newn->next = NULL; 
			last_nugget->next = newn;
			last_nugget = newn;
		}

		n = h->bucketlists[bucket];
		if (n == NULL)
			result = NULL;
		else {
			result = n->addr;
			h->bucketlists[bucket] = n->next;
			nfree(h, n);
		}
	}

	return result;
}


/*
 * heap_free()
 *
 *	Return 'size' bytes of memory pointed to by 'p' to heap 'h'.
 */
void heap_free(heap* h, void* p, int size)
{
	nugget* n;
	int	bucket;

	n = nalloc(h);
	if (n == NULL) {
		fprintf(stderr, "heap_free: nalloc failed.\n");
		return;
	}
	bucket = log2(size);
	bucket = (bucket < BUCKET_MIN ? BUCKET_MIN : bucket);
	n->addr = p;
	n->next = h->bucketlists[bucket];
	h->bucketlists[bucket] = n;
}

