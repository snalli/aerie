#include "mfs/pinode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common/errno.h"

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
}


int PInode::Write(char* src, uint64_t off, uint64_t n)
{


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
			printf("Readblock: node->slots=%p\n", node->slots);
			printf("Readblock: node->slots[%d]=%p\n", offset, node->slots[offset]);
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
PInode::PatchLink(PInode::Link& link)
{
	//sint64_t size = 1 << ((link.slot_height-1)*RADIX_TREE_MAP_SHIFT);

	//printf("link.slot_height=%d\n", link.slot_height);
/*
	if (link.region_.bsize_ != size)
	if (extent->bcount_ == 1) {
		*slot = extent->ptr_;
	} else {
		radixtree = reinterpret_cast<RadixTree*>(extent->ptr_);
		*slot = radixtree->rnode_->slots
	}
*/	
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


int 
PInode::LookupBlock(uint64_t bn, Link* link, Region* region)
{
	uint64_t       rbn;
	RadixTreeNode* node;
	int            slot_offset;
	int            slot_height;
	int            ret;
	uint64_t       size;
	int            i;

	if (bn < N_DIRECT) {
		if (link) {
			link->Init(this, daddrs_, bn, 0);
		}	
		if (region) {
			region->Init(daddrs_[bn], bn, 1);
		}	
	} else {
		rbn = bn - N_DIRECT; 
		if ( (ret = radixtree_.MapSlot(rbn, 0, &node, 
		                               &slot_offset, &slot_height)) != 0) 
		{
			return ret;
		}
		size = 1 << ((slot_height-1)*RADIX_TREE_MAP_SHIFT);
		if (link) {
			link->Init(this, node->slots, slot_offset, slot_height);
		}
		if (region) {
			region->Init(node->slots[slot_offset], 
			             N_DIRECT + (rbn & ~(size-1)), size);
		}	
	}	

	return 0;
}


int 
PInode::InsertRegion(uint64_t bn, Region* region)
{
	uint64_t       rbn;
	RadixTreeNode* node;
	int            slot_offset;
	int            slot_height;
	int            ret;
	uint64_t       size;
	int            i;

	if (bn < N_DIRECT) {
		assert(region->bsize_ == 1);
		daddrs_[bn] = region->dblock_;
	} else {
		rbn = bn - N_DIRECT; 
		if ( (ret = radixtree_.MapSlot(rbn, 0, &node, 
		                               &slot_offset, &slot_height)) != 0) 
		{
			return ret;
		}
		// FIXME: if slot found then we might need to swap the current radixtree with the new radixtree, and put the old radixtree under the new one. 
		// How large the region needs to be? What is the location (block number)?

		size = 1 << ((slot_height-1)*RADIX_TREE_MAP_SHIFT);
		assert(region->bsize_ == size); 	// compatible region
		if (size == 1) {
			node->slots[slot_offset] = region->dblock_;
			printf("InsertRegion: node->slots=%p\n", node->slots);
			printf("InsertRegion: node->slots[%d]=%p\n", slot_offset, node->slots[slot_offset]);
		} else {
			assert(region->radixtree_.rnode_ != NULL);
			assert(region->radixtree_.height_ == slot_height); // do we need this? bsize should cover us
			node->slots[slot_height] = region->radixtree_.rnode_;
		}	
	}	

	return 0;
}


int 
PInode::Region::WriteBlock(uint64_t rbn, char* src, int n)
{
	void**         slot;
	RadixTreeNode* node;
	int            offset;
	int            height;
	int            ret;

	if (rbn > bsize_ - 1) {
		// block number out of range
		return -EINVAL;
	}

	if (bsize_ == 1) {
		slot = &dblock_;
	} else {
		ret = radixtree_.MapSlot(rbn, 1, &node, &offset, &height);
		printf("Writeblock: %d, %d\n", rbn, offset);
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

	return n;
}


int 
PInode::Region::ReadBlock(uint64_t rbn, char* src, int n)
{
	void**         slot;
	RadixTreeNode* node;
	int            offset;
	int            height;
	int            ret;

	if (rbn > bsize_ - 1) {
		// block number out of range
		return -EINVAL;
	}

	if (bsize_ == 1) {
		slot = &dblock_;
	} else {
		if (radixtree_.MapSlot(rbn, 0, &node, &offset, &height) == 0) {
			slot = &node->slots[offset];
		} else {
			*slot = NULL;
		}
	}
	if (*slot == (void*)0) {
		memset(src, 0, n); 
	} else {
		memmove(src, *slot, n);
	}
	return n;
}
