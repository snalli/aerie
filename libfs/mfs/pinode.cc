#include "mfs/pinode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define min(a,b) ((a) < (b)? (a) : (b))


inline uint64_t 
block_valid_data(int bn, uint64_t file_size)
{
	return ((((bn + 1) * BLOCK_SIZE) < file_size ) 
	        ? BLOCK_SIZE 
	        : (file_size - (bn * BLOCK_SIZE)));
}


PInode::PInode()
{
	for (int i=0; i<N_DIRECT; i++) {
		daddrs_[i] = (void*)0;
	}
	radixtree_.Extend(RADIX_TREE_MAP_SIZE-1); // Make sure the radixtree has at least height 1
}


uint64_t 
PInode::LookupBlock(uint64_t bn)
{
	void* block;

/*
	if (bn < N_DIRECT) {
		*dst = reinterpret_cast<char*>(daddrs_[bn]); 
		if (daddrs_[bn]) {
			printf("%d\n", bn);
			return block_valid_data(bn, size_);
		} else {
			return 0;
		}
	} else {
		bn = bn - N_DIRECT; 
		if (block = radixtree_.Lookup(bn)) {
			*dst = reinterpret_cast<char*>(block); 
			return block_valid_data(bn, size_);
		} else {
			return 0;
		}
	}
*/	
}


int 
PInode::InsertBlock(void* block, uint64_t bn, int n)
{
	void*  dst;
	void** slot;

	if (bn < N_DIRECT) {
		slot = &daddrs_[bn];
	} else {
		//FIXME: check size first: if file_size/BLOCK_SIZE smaller than block number
		//then you don't need to lookup the radixtree. But this requires keeping
		//track the file size correctly. need to fix this. 
		bn = bn - N_DIRECT; 
		if (slot = radixtree_.LookupSlot(bn)) {
			slot = &daddrs_[bn];
		} else {
			
			printf("Insert\n");
			return 0;
		}
	}
}


int 
PInode::ReadBlock(uint64_t bn, char* dst, int n)
{
	uint64_t       rbn;
	void*          item;
	RadixTreeNode* node;
	int            offset;
	int            height;
	int            s;

	if (bn < N_DIRECT) {
		item = daddrs_[bn];
	} else {
		rbn = bn - N_DIRECT; 
		if (radixtree_.MapSlot(rbn, 0, &node, &offset, &height) == 0) {
			item = node->slots[offset];
		} else {
			item = NULL;
		}
	}
	if (!item) {
		if (bn*BLOCK_SIZE < size_) {
			memset(dst, 0, n);
			return n;
		}
		return -1;
	}
	s = min(BLOCK_SIZE, size_ - bn*BLOCK_SIZE);
	n = min(n,s);
	memmove(dst, item, n);

	return n;
}


// Physically link an extent to an inode slot.
int 
PInode::PhysicalLink(void** slot, Extent* extent)
{
	RadixTree* radixtree;

	if (extent->bcount_ == 1) {
		*slot = extent->ptr_;
	} else {
		radixtree = reinterpret_cast<RadixTree*>(extent->ptr_);
		*slot = radixtree->rnode_->slots
	}
	return 0;
}

int 
PInode::WriteBlock(uint64_t bn, char* src, int n)
{
	uint64_t       rbn;
	void**         slot;
	RadixTreeNode* node;
	int            offset;
	int            height;
	int            ret;

	if (bn < N_DIRECT) {
		slot = &daddrs_[bn];
	} else {
		rbn = bn - N_DIRECT; 
		ret = radixtree_.MapSlot(rbn, 1, &node, &offset, &height);
		assert(ret == 0);
		slot = &node->slots[offset];
	}
	if (*slot == (void*)0) {
		//FIXME: we should allocate a chunk instead of using malloc
		*slot = malloc(BLOCK_SIZE);
		memset(((char*)*slot)+n, 0, BLOCK_SIZE-n); // zero the part of the block 
		                                           // that won't be written
	}
	memmove(*slot, src, n);

	if ( (bn*BLOCK_SIZE + n) > size_ ) {
		size_ = bn*BLOCK_SIZE + n;
	}
	return n;
}
