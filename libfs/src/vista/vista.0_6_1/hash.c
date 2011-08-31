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
 * hash.c
 *
 * DESCRIPTION
 *
 *   This file contains the source for our hash table routines. Our hash
 *   tables use a linear chaining collision resolution scheme. Insertion
 *   and deletion into these tables are atomic operations.
 *
 * PUBLIC FUNCTIONS
 *
 *   void vista_hash_init(hash_table* t, int key_type, heap* h)
 *   void vista_hash_insert(hash_table* t, char* key, void* val)
 *   void* vista_hash_find(hash_table* t, char* key)
 *   void vista_hash_delete_entry(hash_table* t, char* key)
 *   void vista_hash_delete_table(hash_table* t)
 *   void vista_hash_stats(hash_table *t)
 *
 * AUTHOR
 *   Dave Lowell
 */

/**
 ** Includes
 **/

#include <stdio.h>
#include <stdlib.h>
#include "heap.h"
#include "hash.h"


/**
 ** Functions
 **/

/*
 * Our internal memory allocator that uses the appropriate heap, and keeps
 * track of the ammount of memory allocated.
 */
static void* h_alloc(hash_table* t, int size)
{
	void	*res;

	if (t->heap == NULL) {
		res = malloc(size);
	}
	else {
		res = heap_malloc(t->heap, size);
	}
	if (res == NULL) {
		fprintf(stderr, "hash: out of memory\n");
		exit(1);
	}
	else {
		t->space += size;
		return res;
	}
}

/*
 * Routine to free internal memory to the appropriate heap. 
 */
static void h_free(hash_table* t, void *p, int size)
{
	t->space -= size;
	if (t->heap == NULL) {
		free((char*)p);
	}
	else {
		heap_free(t->heap, (char*)p, size);
	}
}

/*
 * This routine computes a hash value for string keys. This string hash
 * function is taken from Tcl/Tk's hash tables.
 */
static int string_index(hash_table *t, char *str)
{
	int	result, i;

	for (result=0, i=0; str[i] != 0; i++) 
		result += (result<<3) + str[i];
	return (result % t->num_slots);
}

/*
 * This routines idempotently switches a hash table to its new expanded 
 * form, and then atomically marks the swap complete.
 */
static void safe_table_swap(hash_table *t)
{
	hash_table	*new, *old;

	if (t->new_table == NULL) return;

	new = t->new_table;

	t->table 	= new->table;
	t->num_slots 	= new->num_slots;
	t->num_entries 	= new->num_entries;
	t->rebuild_size = new->rebuild_size;
	t->key_type 	= new->key_type;
	t->space 	= new->space;
	t->heap 	= new->heap;

	/*
	 * This next assignment marks the end of the swap. Everything
	 * up to now is idempotent. Everthing after this is carefully ordered
	 * so that crashes can occur without corrupting the hash table.
	 */
	t->new_table	= NULL;

	/*
	 * Crashes in the middle of these next steps will leak stuff, but
	 * that's not a catastrophe.
	 */
	h_free(t->old_table, new, sizeof(*new));
	vista_hash_delete_table(t->old_table);
	h_free(t->old_table, t->old_table, sizeof(*new));
	t->old_table = NULL;
}

/*
 * This function rebuilds the hash table, increasing its size by a factor of
 * REBUILD_FACT. Expanding and rehashing all the entries in the table is
 * obviously a difficult task to do atomically. So we will rehash all the
 * entries into an entirely new table, then atomically swap the new
 * table for the old, and finally free the old table.
 */
static void expand_table(hash_table *t)
{
	int		i;
	hash_table	*new, *old;
	h_node		*n;

	/*
	 * Create a copy of the old table
	 */
	old = (hash_table*) h_alloc(t, sizeof(*t));
	bcopy(t, old, sizeof(*t));

	/*
	 * Allocate, and manually configure new hash table
	 */
	new 		  = (hash_table*) h_alloc(t, sizeof(*new));
	new->space        = 0;
	new->heap         = t->heap;
	new->num_slots 	  = t->num_slots * REBUILD_FACT;
	new->table 	  = (h_node**) h_alloc(new, new->num_slots * sizeof(n));
	new->num_entries  = 0;
	new->rebuild_size = t->rebuild_size * REBUILD_FACT;
	new->key_type     = t->key_type;
	new->new_table    = NULL;
	new->old_table    = NULL;

	for (i = 0; i < new->num_slots; i++)
		new->table[i] = NULL;

	/*
	 * Create a new node in the new table for each node in the old
	 * table.
	 */
	for (i = 0; i < t->num_slots; i++) {
		for (n = t->table[i]; n != NULL; n = n->next)
			vista_hash_insert(new, n->key, n->data);
	}

	/*
	 * Set things up for the atomic table swap.
	 */
	t->old_table = old;

	/*
	 * Swap.
	 */
	t->new_table = new;

	/*
	 * Idempotently free old table.
	 */
	safe_table_swap(t);
}

/*
 * void vista_hash_init(hash_table* t, int key_type, heap* h)
 *
 *   t -		hash table to initialize
 *   key_type -		string or one word keys
 *   h -		heap to use for allocating internal data
 *
 *   Initialize hash table 't' to use the specified key type (either
 *   HASH_STRING_KEYS or HASH_ONE_WORD_KEYS), and the specified heap for
 *   allocating internal data. The heap parameter can be NULL to force
 *   internal data to be allocated using the standard allocator in libc.
 */
void vista_hash_init(hash_table* t, int key_type, heap* h)
{
	int	i;

	t->space	= 0;
	t->heap 	= h;
	t->table 	= h_alloc(t, sizeof(h_node*) * INIT_SIZE);
	t->num_slots	= INIT_SIZE;
	t->num_entries	= 0;
	t->rebuild_size = INIT_SIZE * PER_SLOT_MAX;
	t->key_type	= key_type;
	t->new_table	= NULL;
	t->old_table	= NULL;

	for (i=0; i < INIT_SIZE; i++)
		t->table[i] = NULL;
}

/*
 * void vista_hash_insert(hash_table* t, char* key, volatile void* val)
 *
 *   t - 		hash table
 *   key -		key for indexing item inserted into hash table
 *   val -		value to insert into hash table
 *
 *   Insert the value 'val' into the hash table 't', indexed by the value
 *   'key'. The index key can be either a string or a single word value
 *   depending on how the hash table was initialized.
 */
void vista_hash_insert(hash_table* t, char* key, volatile void* val)
{
	int	index;
	h_node 	*new;

	CHECK_TABLE(t);
	
	new = (h_node*) h_alloc(t, sizeof(h_node));
	new->data = (void *) val;
	new->next = NULL;

	if (t->key_type == HASH_STRING_KEYS) {
		new->key = h_alloc(t, strlen(key)+1);
		strcpy(new->key, key);
	}
	else {
		new->key = key;
	}

	index = HASH_INDEX(t, key);

	/*
	 * Atomically insert new node in list
	 */
	if (t->table[index] != NULL) {
		new->next = t->table[index];
	}
	t->table[index] = new;

	t->num_entries++;
	if (t->num_entries >= t->rebuild_size) {
		/*
		 * The hash table is getting full, so atomically rebuild
		 * the table with more slots.
		 */
		expand_table(t);
	}
}

/*
 * void* vista_hash_find(hash_table* t, char* key)
 *
 *   t - 		hash table
 *   key -		key for indexing item in hash table
 *
 *   Return the value in the hash table 't' indexed by 'key', or NULL if 
 *   no such item exists in the table.
 */
void* vista_hash_find(hash_table* t, char* key)
{
	int	i;
	h_node	*n;

	CHECK_TABLE(t);
	
	i = HASH_INDEX(t, key);

	for (n=t->table[i]; n != NULL && !HASH_MATCH(t,n->key,key); n=n->next);

	return (n == NULL ? NULL : n->data);
}

/*
 * void* vista_hash_delete_entry(hash_table* t, char* key)
 *
 *   t - 		hash table
 *   key -		key of item to delete in table
 *
 *   Delete the item in hash table 't' indexed by 'key'. This function does
 *   nothing if the indexed item does not exist in the table.
 */
void vista_hash_delete_entry(hash_table* t, char* key)
{
	int	index;
	h_node	*p, *n;

	CHECK_TABLE(t);
	
	index = HASH_INDEX(t, key);

	for (p = NULL, n = t->table[index]; n != NULL; p = n, n = n->next) {
		if (HASH_MATCH(t, n->key, key)) break;
	}
	if (n == NULL) return;
	if (p == NULL) {
		t->table[index] = n->next;
		if (t->key_type == HASH_STRING_KEYS)
			h_free(t, n->key, strlen(n->key)+1);
	}
	else {
		p->next = n->next;
	}
	h_free(t, n, sizeof(*n));
	t->num_entries--;
}

/*
 * void vista_hash_delete_table(hash_table* t)
 *
 *   t - 		hash table to delete
 *
 *   Free all the internal storage associated with this table. The user is
 *   responsible for freeing any user-allocated data pointed to by table
 *   entries. The only valid operation on a hash table that has been deleted
 *   is initialization with vista_hash_init().
 */
void vista_hash_delete_table(hash_table* t)
{
	int	i;
	h_node	*n, *p;

	CHECK_TABLE(t);
	
	for (i = 0; i < t->num_slots; i++) {
		for (n = t->table[i]; n != NULL; n = p) {
			p = n->next;
			if (t->key_type == HASH_STRING_KEYS)
				h_free(t, n->key, strlen(n->key)+1);
			h_free(t, n, sizeof(*n));
		}
	}
	h_free(t, t->table, t->num_slots * sizeof(h_node*));
}

/*
 * void vista_hash_stats(hash_table *t)
 *
 *   t - 		hash table
 *
 *   Print out useful statistics concerning hash table 't'.
 */
void vista_hash_stats(hash_table *t)
{
	int	i, over, count, hist[MAX_HISTOGRAM];
	int	unused, len, j, chains, chainlengths;
	float	chain_ave;
	h_node	*n;
	char	s[64];

	fprintf(stderr, "\n\t\t\tHash Table Info:\n\n");
	fprintf(stderr, "\t\tTable size:\t\t%d\n", t->num_slots);
	fprintf(stderr, "\t\tNumber of entries:\t%d\n", t->num_entries);
	fprintf(stderr, "\t\tNext rebuild size:\t%d\n", t->rebuild_size);
	fprintf(stderr, "\t\tMem used for metadata:\t%d bytes\n", t->space);
	fprintf(stderr, "\t\tKey type:\t\t%s\n", (t->key_type==HASH_STRING_KEYS?"strings":"integer"));
	fprintf(stderr, "\t\tHeap type:\t\t%s\n", (t->heap==NULL?"libc malloc":"local malloc"));

	bzero(hist, MAX_HISTOGRAM * sizeof(int));

	over = 0;
	chains = 0;
	chainlengths = 0;

	for (i=0; i < t->num_slots; i++) {
		for (count=0, n=t->table[i]; n != NULL; n=n->next, count++);
		if (count != 0) {
			chains++;
			chainlengths += count;
		}
		if (count < MAX_HISTOGRAM) {
			hist[count]++;
		}
		else {
			over++;
		}
	}

	chain_ave = 0.0;
	if (chains > 0) 
		chain_ave = ((float)chainlengths)/((float)chains);
		

	fprintf(stderr, "\t\tAverage chain length:\t%.1f\n", chain_ave);

	fprintf(stderr, "\n\t\t\tChain Length Histogram:\n\n");

	for (i=0, len=0; i < MAX_HISTOGRAM; i++) {
		fprintf(stderr, "%7d", i);
	}
	fprintf(stderr, "%7s\n", "over");
	for (i=0; i < MAX_HISTOGRAM+1; i++)
		fprintf(stderr, "%7s", "-");
	fprintf(stderr, "\n");
	for (i=0; i < MAX_HISTOGRAM; i++) {
		fprintf(stderr, "%7d", hist[i]);
	}
	fprintf(stderr, "%7d\n", over);
	fprintf(stderr, "\n");
}


