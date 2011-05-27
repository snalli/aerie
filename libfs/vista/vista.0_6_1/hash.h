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
 * hash.h
 *
 * DESCRIPTION
 *
 *   This file contains the type and constant declarations for our hash
 *   table routines.
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

#ifndef HASH__H
#define HASH__H

/**
 ** Includes
 **/

#include "heap.h"


/**
 ** Macros
 **/

#define INIT_SIZE	16	/* must be power of 2 */
#define PER_SLOT_MAX	2	/* ave chain length -> rebuild */
#define REBUILD_FACT	4	/* Size increase factor. Must be power of 2. */
#define MAX_HISTOGRAM 	10	/* max chain length in stats histogram */

#define HASH_ONE_WORD_KEYS 	10
#define HASH_STRING_KEYS	11

#define INT_INDEX(t, i) (((i)*1103515245) & ((t)->num_slots-1))

#define HASH_INDEX(t, k) ((t)->key_type==HASH_STRING_KEYS ? string_index(t, k) \
			 : INT_INDEX(t, (int)k))

#define HASH_MATCH(t, k1, k2) ((t)->key_type == HASH_STRING_KEYS ? \
		            (strcmp(k1, k2)==0) : (k1==k2))

#define CHECK_TABLE(t) if ((t)->new_table != NULL) safe_table_swap(t)


/**
 ** Types
 **/

typedef struct h_node_s {
	void	*key;
	void	*data;
	struct h_node_s *next;
} h_node;

typedef struct hash_table_s {
	h_node		**table;
	int		num_slots;
	int		num_entries;
	int		rebuild_size;
	int		key_type;
	int		space;
	heap		*heap;

	/* some extra stuff is needed for atomic rebuild */
	struct hash_table_s	*new_table; 
	struct hash_table_s	*old_table; 
} hash_table;


/**
 ** Module Functions
 **/

extern void vista_hash_init(hash_table*, int, heap*);
extern void vista_hash_insert(hash_table*, char*, volatile void*);
extern void* vista_hash_find(hash_table*, char*);
extern void vista_hash_delete_entry(hash_table*, char*);
extern void vista_hash_delete_table(hash_table*);
extern void vista_hash_stats(hash_table*);

#endif

