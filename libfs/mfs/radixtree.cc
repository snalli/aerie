/*
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

#include "mfs/radixtree.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "common/errno.h"


#define BUG_ON(x) 

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

struct radix_tree_path {
	struct radix_tree_node* node;
	int                     offset;
};

#define RADIX_TREE_MAX_PATH (DIV_ROUND_UP(RADIX_TREE_INDEX_BITS, \
					  RADIX_TREE_MAP_SHIFT))


#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


// FIXME: the following is about the representation of pointers in the Linux
// original radix tree. We change this to use chunk descriptors.

/*
 * An indirect pointer (root->rnode pointing to a radix_tree_node, rather
 * than a data item) is signalled by the low bit set in the root->rnode
 * pointer.
 *
 * In this case root->height is > 0, but the indirect pointer tests are
 * needed for RCU lookups (because root->height is unreliable). The only
 * time callers need worry about this is when doing a lookup_slot under
 * RCU.
 */
#define RADIX_TREE_RETRY ((void *)-1UL)



static inline int radix_tree_is_indirect_ptr(void *ptr)
{
	//FIXME: use chunk descriptors instead of pointers
	return (int)((unsigned long)ptr & RADIX_TREE_INDIRECT_PTR);
}


/*
 * The height_to_maxindex array needs to be one deeper than the maximum
 * path as height 0 holds only 1 entry.
 */
static uint64_t height_to_maxindex[RADIX_TREE_MAX_PATH + 1];

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


//static void 
void
radix_tree_init_maxindex(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(height_to_maxindex); i++) {
		height_to_maxindex[i] = __maxindex(i);
	}	



}


/*
 *	Return the maximum key which can be stored into a
 *	radix tree with height HEIGHT.
 */
inline uint64_t 
RadixTree::MaxIndex(unsigned int height)
{
	return height_to_maxindex[height];
}


static struct RadixTreeNode*
radix_tree_node_alloc(RadixTree* root)
{
	RadixTreeNode* node = new RadixTreeNode;

	return node;
}


RadixTree::RadixTree(): 
	height_(0), rnode_(NULL)
{ }


/*
 *	Extend a radix tree so it can store key @index.
 */
int 
RadixTree::Extend(uint64_t index)
{
	RadixTreeNode* node;
	unsigned int   h;

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
		if (!(node = radix_tree_node_alloc(this))) {
			return -ENOMEM;
		}

		// Increase the height.
		node->slots[0] = radix_tree_indirect_to_ptr(rnode_);

		newheight = height_ + 1;
		node = reinterpret_cast<RadixTreeNode*> (radix_tree_ptr_to_indirect(reinterpret_cast<void*>(node)));
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
int RadixTree::MapSlot(uint64_t index, int alloc, RadixTreeNode** result_node, int* result_offset, int* result_height)
{
	RadixTreeNode* node = NULL;
	RadixTreeNode* slot;
	unsigned int   h;
	unsigned int   shift;
	int            offset;
	int            error;

	BUG_ON(radix_tree_is_indirect_ptr(item));

	// Special boundary case
	// If index == 0 and height_ == 0, we have two options:
	// 1) Use &rnode_ as the slot node. In this case:
	//	  *result_node = reinterpret_cast<RadixTreeNode*>(&rnode_);
	//	  OR
	// 2) Extent the tree to height=1 to allocate the slot in an 
	//    indirect block
	// We prefer option (2) because it simplifies PInode

	// Make sure the tree is high enough.
	if (index > MaxIndex(height_) || (index == 0 && height_== 0)) {
		if (alloc) {
			error = Extend(index);
			if (error) {
				return error;
			}	
		} else {
			return -1;
		}
	}
	
	slot = reinterpret_cast<RadixTreeNode*>(radix_tree_indirect_to_ptr(reinterpret_cast<void*>(rnode_)));
	h = height_;
	shift = (h-1) * RADIX_TREE_MAP_SHIFT;
	offset = 0;			// uninitialised var warning
	while (h > 0) {
		if (slot == NULL) {
			if (alloc) {
				// Have to add a child node.
				if (!(slot = radix_tree_node_alloc(this))) {
					return -ENOMEM;
				}	
				if (node) {
					node->slots[offset] = slot;
				} else {
					rnode_ = reinterpret_cast<RadixTreeNode*> (radix_tree_ptr_to_indirect(reinterpret_cast<void*>(slot)));
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
		slot = reinterpret_cast<RadixTreeNode*> (node->slots[offset]);
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
///  @root:    radix tree root
///  @index:   index key
///  @item:    item to insert
///
///  Insert an item into the radix tree at position @index.
///
int RadixTree::Insert(uint64_t index, void* item)
{
	RadixTreeNode* node;
	RadixTreeNode* slot;
	int            offset;
	int            height;
	int            ret;
	
	if ((ret=MapSlot(index, 1, &node, &offset, &height))) {
		return ret;
	}

	assert(height <= 1);
	slot = reinterpret_cast<RadixTreeNode*>(node->slots[offset]);

	if (slot != NULL) {
		return -EEXIST;
	}
	
	node->slots[offset] = item;

	return 0;
}


///
///  RadixTree::LookupSlot    -    lookup a slot in a radix tree
///  @root:     radix tree root
///  @index:    index key
///
///  Returns:  the slot corresponding to the position @index in the
///  radix tree @root. This is useful for update-if-exists operations.
///
void** RadixTree::LookupSlot(uint64_t index)
{
	RadixTreeNode** slot;
	RadixTreeNode*  node;
	int             offset;
	int             height;
	int             ret;

	if (!(ret = MapSlot(index, 0, &node, &offset, &height))) {
		if (height <= 1) {
			slot = reinterpret_cast<RadixTreeNode**>(&node->slots[offset]);
			return (void **) slot;
		}	
	}
	return (void **) 0;
}


///
///  RadixTree::Lookup    -    perform lookup operation on a radix tree
///  @root:     radix tree root
///  @index:    index key
///
///  Lookup the item at the position @index in the radix tree @root.
///
void* RadixTree::Lookup(uint64_t index)
{
	RadixTreeNode** slot;
	RadixTreeNode*  node;
	int             offset;
	int             height;
	int             ret;

	if (!(ret = MapSlot(index, 0, &node, &offset, &height))) {
		if (height <= 1) {
			return node->slots[offset];
		}	
	}
	return (void *) 0;
}
