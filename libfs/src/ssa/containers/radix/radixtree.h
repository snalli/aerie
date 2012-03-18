/**
 * \brief Based on Linux radix tree.
 *
 */


/*
 * Based on Linux radix tree. Copyright license below.
 *
 * 
 * Copyright (C) 2001 Momchil Velikov
 * Portions Copyright (C) 2001 Christoph Hellwig
 * Copyright (C) 2005 SGI, Christoph Lameter
 * Copyright (C) 2006 Nick Piggin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// TODO: any allocations and assignments done by Mapslot must be journalled

#ifndef __STAMNOS_SSA_COMMON_RADIXTREE_H
#define __STAMNOS_SSA_COMMON_RADIXTREE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <typeinfo>
#include "common/errno.h"
#include "bcs/main/common/cdebug.h"
#include "spa/const.h"


#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

#define RADIX_TREE_MAX_PATH (DIV_ROUND_UP(RADIX_TREE_INDEX_BITS, \
					  RADIX_TREE_MAP_SHIFT))


#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


const int      BITS_PER_LONG         = 64;
const int      LONG_SHIFT            = 3;
const int      RADIX_TREE_MAP_SHIFT  = kBlockShift - LONG_SHIFT;
const uint64_t RADIX_TREE_MAP_SIZE   = (1ULL << RADIX_TREE_MAP_SHIFT);
const uint64_t RADIX_TREE_MAP_MASK   = (RADIX_TREE_MAP_SIZE-1);
const int      RADIX_TREE_INDEX_BITS = (8 /* CHAR_BIT */ * sizeof(uint64_t));


static inline void *radix_tree_indirect_to_ptr(void *ptr)
{
	return ptr;
}

static inline void *radix_tree_ptr_to_indirect(void *ptr)
{
	return ptr;
}

// NOTE: A RadixTreeNode has no count field. We made this design decision
// so as to pack more slots in the node instead. I don't expect count to be 
// useful because we rarely delete indirect nodes when using radixtree as 
// a representation for a file. Note that the existence of an indirect
// block indicates the existence of at least one linked slot.

template<typename Session>
class RadixTreeNode {
public:
	RadixTreeNode()
	{
		memset(&slots, 0, RADIX_TREE_MAP_SIZE*sizeof(*slots));
	}

	void* operator new(size_t nbytes, Session* session)
	{
		void* ptr;
		if (session->salloc()->Alloc(session, nbytes, typeid(RadixTreeNode<Session>), &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}

	int Link(Session* session, int slot_index, void* item)
	{
		slots[slot_index] = item;
		return E_SUCCESS;
	}

	int Link(Session* session, int slot_index, RadixTreeNode* node)
	{
		RadixTreeNode* n;
		n = reinterpret_cast<RadixTreeNode*>(radix_tree_indirect_to_ptr(reinterpret_cast<void*>(node)));
		return Link(session, slot_index, (void*) n);
	}

	RadixTreeNode* Slot(Session* session, int slot_index) 
	{
		return reinterpret_cast<RadixTreeNode<Session> *> (slots[slot_index]);
	}

	void* slots[RADIX_TREE_MAP_SIZE];
};


template<typename Session>
class RadixTree {
public:
	RadixTree();
	inline uint64_t MaxIndex(unsigned int height);
	int Extend(Session* session, unsigned long index);
	int Insert(Session* session, uint64_t index, void *item, int);
	void* Lookup(Session* session, uint64_t index, int);
	void** LookupSlot(Session* session, uint64_t index, int);
	int MapSlot(Session* session, uint64_t, int, int, RadixTreeNode<Session>**, int*, int*);
	int LookupLowestSlot(Session* session, uint64_t, RadixTreeNode<Session>**, int*, int*);

	inline RadixTreeNode<Session>* get_rnode() {	return rnode_; }


	unsigned int            height_;
	RadixTreeNode<Session>* rnode_;
};


template<typename Session>
RadixTree<Session>::RadixTree()
	: height_(0), 
	  rnode_(NULL)
{ }


static uint64_t 
__maxindex(unsigned int height)
{
	unsigned int width = height * RADIX_TREE_MAP_SHIFT;
	int shift = RADIX_TREE_INDEX_BITS - width;

	if (shift < 0)
		return ~0ULL;
	if (shift >= BITS_PER_LONG)
		return 0ULL;
	return ~0ULL >> shift;
}


static void
radix_tree_init_maxindex(uint64_t height_to_maxindex[], int array_size)
{
	unsigned int i;

	for (i = 0; i < array_size; i++) {
		height_to_maxindex[i] = __maxindex(i);
	}	
}


/*
 *	Return the maximum key which can be stored into a
 *	radix tree with height HEIGHT.
 */
template<typename Session> 
uint64_t 
RadixTree<Session>::MaxIndex(unsigned int height)
{
	/*
	 * The height_to_maxindex array needs to be one deeper than the maximum
	 * path as height 0 holds only 1 entry.
	 */
	static uint64_t height_to_maxindex[RADIX_TREE_MAX_PATH + 1];
	static bool table_valid = false;

	if (!table_valid) {
		radix_tree_init_maxindex(height_to_maxindex, ARRAY_SIZE(height_to_maxindex));
		table_valid = true;
	}
	return height_to_maxindex[height];
}


/*
 *	Extend a radix tree so it can store key @index.
 */
template<typename Session>
int 
RadixTree<Session>::Extend(Session* session, uint64_t index)
{
	RadixTreeNode<Session>* node;
	unsigned int            h;
	int                     ret;

	// Figure out what the height should be.
	h = height_ + 1;
	while (index > MaxIndex(h)) {
		h++;
	}	

	if (rnode_ == NULL) {
		height_ = h;
		goto out;
	}

	do {
		unsigned int newheight;
		if (!(node = new(session) RadixTreeNode<Session>())) {
			return -E_NOMEM;
		}

		// Increase the height.
		node->Link(session, 0, rnode_);

		newheight = height_ + 1;
		node = reinterpret_cast<RadixTreeNode<Session> *> (radix_tree_ptr_to_indirect(reinterpret_cast<void*>(node)));
		rnode_ = node;
		height_ = newheight;
	} while (h > height_);
out:
	return 0;
}


//
// Returns -1 if:
// 1) Root node is null (has not been allocated yet), or
// 2) INDEX is larger than the maximum index that can mapped by the current tree
//
// We could allow INDEX 0 be mapped on the root node even if the root is NULL
// but we don't for simplifying the boundary conditions
//
template<typename Session>
int 
RadixTree<Session>::MapSlot(Session* session, 
                            uint64_t index, 
                            int min_height, 
                            int alloc, 
                            RadixTreeNode<Session>** result_node, 
                            int* result_offset, int* result_height)
{
	RadixTreeNode<Session>* node = NULL;
	RadixTreeNode<Session>* slot;
	unsigned int            h;
	unsigned int            shift;
	int                     offset;
	int                     error;

	// Special boundary case
	// If index == 0 and height_ == 0, we have two options:
	// 1) Use &rnode_ as the slot node. In this case:
	//	  *result_node = reinterpret_cast<RadixTreeNode*>(&rnode_);
	//	  OR
	// 2) Extent the tree to height=1 to allocate the slot in an 
	//    indirect block
	//
	// We let the callee indicate this via min_height 
	// If min_height == 0 then option (1) is used, otherwise option (2)

	if (min_height == 0 && index == 0 && height_ == 0) {
		*result_node = reinterpret_cast<RadixTreeNode<Session> *>(&rnode_);
		*result_offset = 0;
		*result_height = 0;
		return 0;
	} else {
		// Make sure the tree is high enough.
		if (index > MaxIndex(height_) || (index == 0 && height_== 0)) {
			if (alloc) {
				error = Extend(session, index);
				if (error) {
					return error;
				}	
			} else {
				return -1;
			}
		}
	}
	
	slot = reinterpret_cast<RadixTreeNode<Session> *>(radix_tree_indirect_to_ptr(reinterpret_cast<void*>(rnode_)));
	h = height_;
	shift = (h-1) * RADIX_TREE_MAP_SHIFT;
	offset = 0;			// uninitialised var warning
	while (h > 0) {
		if (slot == NULL) {
			if (alloc) {
				// Have to add a child node.
				if (!(slot = new(session) RadixTreeNode<Session>())) {
					return -E_NOMEM;
				}	
				if (node) {
					node->Link(session, offset, slot);
				} else {
					rnode_ = reinterpret_cast<RadixTreeNode<Session> *> (radix_tree_ptr_to_indirect(reinterpret_cast<void*>(slot)));
				}	
			} else {
				if (node) {
					*result_node = node;
					*result_offset = offset;
					*result_height = h+1;
					return 0;
				}
				return -1;
			}
		}

		// Go a level down
		offset = (index >> shift) & RADIX_TREE_MAP_MASK;
		node = slot;
		printf("radixtree::mapslot: slot=%p, offset=%d\n", node, offset);
		slot = node->Slot(session, offset);
		shift -= RADIX_TREE_MAP_SHIFT;
		h--;
	}

	if (node) {
		*result_node = node;
		*result_offset = offset;
		*result_height = h+1;
		return 0;
	}

	return -1;
}


///
///  radix_tree_insert    -    insert into a radix tree
///  @root:       radix tree root
///  @index:      index key
///  @item:       item to insert
///  @min_height: minimum height of the tree
///
///  Insert an item into the radix tree at position @index.
///
template<typename Session>
int 
RadixTree<Session>::Insert(Session* session, uint64_t index, void* item, 
                           int min_height)
{
	RadixTreeNode<Session>* node;
	RadixTreeNode<Session>* slot;
	int                     offset;
	int                     height;
	int                     ret;
	
	if ((ret=MapSlot(session, index, min_height, 1, &node, &offset, &height))) {
		return ret;
	}

	assert(height <= 1);
	
	if ((slot = node->Slot(session, offset)) != NULL) {
		return -E_EXIST;
	}
	node->Link(session, offset, item);

	return 0;
}


///
///  RadixTree::LookupSlot    -    lookup a slot in a radix tree
///  @root:       radix tree root
///  @index:      index key
///  @min_height: minimum height of the tree
///
///  Returns:  the slot corresponding to the position @index in the
///  radix tree @root. This is useful for update-if-exists operations.
///
template<typename Session>
void** 
RadixTree<Session>::LookupSlot(Session *session, uint64_t index, int min_height)
{
	RadixTreeNode<Session>** slot;
	RadixTreeNode<Session>*  node;
	int                      offset;
	int                      height;
	int                      ret;

	if (!(ret = MapSlot(session, index, min_height, 0, &node, &offset, &height))) {
		if (height <= 1) {
			slot = reinterpret_cast<RadixTreeNode<Session>**>(&node->slots[offset]);
			return (void **) slot;
		}	
	}
	return (void **) 0;
}


///
///  RadixTree::Lookup    -    perform lookup operation on a radix tree
///  @root:       radix tree root
///  @index:      index key
///  @min_height: minimum height of the tree
///
///  Lookup the item at the position @index in the radix tree @root.
///
template<typename Session>
void* 
RadixTree<Session>::Lookup(Session* session, uint64_t index, int min_height)
{
	RadixTreeNode<Session>** slot;
	RadixTreeNode<Session>*  node;
	int                      offset;
	int                      height;
	int                      ret;

	if (!(ret = MapSlot(session, index, min_height, 0, &node, &offset, &height))) {
		if (height <= 1) {
			return node->slots[offset];
		}	
	}
	return (void *) 0;
}


#endif /* __STAMNOS_SSA_COMMON_RADIXTREE_H */
