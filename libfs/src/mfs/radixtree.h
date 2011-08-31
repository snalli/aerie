#ifndef _RADIX_TREE_H_GHA678
#define _RADIX_TREE_H_GHA678

#include <stdint.h>
#include "mfs/mfs_i.h"

const int      BITS_PER_LONG         = 64;
const int      LONG_SHIFT            = 3;
const int      RADIX_TREE_MAP_SHIFT  = BLOCK_SHIFT - LONG_SHIFT;
const uint64_t RADIX_TREE_MAP_SIZE   = (1ULL << RADIX_TREE_MAP_SHIFT);
const uint64_t RADIX_TREE_MAP_MASK   = (RADIX_TREE_MAP_SIZE-1);
const int      RADIX_TREE_INDEX_BITS = (8 /* CHAR_BIT */ * sizeof(uint64_t));


#define RADIX_TREE_INDIRECT_PTR	1
static inline void *radix_tree_indirect_to_ptr(void *ptr)
{
	//FIXME: use chunk descriptors instead of pointers
	return ptr;
	//return (void *)((unsigned long)ptr & ~RADIX_TREE_INDIRECT_PTR);
}

static inline void *radix_tree_ptr_to_indirect(void *ptr)
{
	//FIXME: use chunk descriptors instead of pointers
	return ptr;
	//return (void *)((unsigned long)ptr | RADIX_TREE_INDIRECT_PTR);
}

// NOTE: A RadixTreeNode has no count field. We made this design decision
// so as to pack more slots in the node instead. I don't expect count to be 
// useful because we rarely delete indirect nodes when using it as 
// a representation for a file. Note that the existence of an indirect
// block indicates the existence of at least one linked slot.

class RadixTreeNode {
public:
	void*         slots[RADIX_TREE_MAP_SIZE];

	inline int Link(int slot_index, RadixTreeNode* node)
	{
		RadixTreeNode* n;
		n = reinterpret_cast<RadixTreeNode*>(radix_tree_indirect_to_ptr(reinterpret_cast<void*>(node)));
		slots[slot_index] = (void*) n;
	}
};


class RadixTree {

public:
	unsigned int   height_;
	RadixTreeNode* rnode_;

	RadixTree();
	inline uint64_t MaxIndex(unsigned int height);
	int Extend(unsigned long index);
	int Insert(uint64_t index, void *item, int);
	void* Lookup(uint64_t index, int);
	void** LookupSlot(uint64_t index, int);
	int MapSlot(uint64_t, int, int, RadixTreeNode**, int*, int*);
	int LookupLowestSlot(uint64_t, RadixTreeNode**, int*, int*);

	inline RadixTreeNode* get_rnode() {	return rnode_; }
};


#endif /* _RADIX_TREE_H_GHA678 */
