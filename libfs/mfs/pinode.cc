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


int 
PInode::InsertRegion(Region* region)
{
	uint64_t newheight;
	uint64_t bcount;

	printf("PInode::InsertRegion region=%p\n", region);
	printf("PInode::InsertRegion region->radixtree_.rnode_=%p\n", region->radixtree_.rnode_);
	printf("PInode::InsertRegion region->base_bn_=%d\n", region->base_bn_);
	printf("PInode::InsertRegion region->slot_.slot_base_=%p\n", region->slot_.slot_base_);

	if (region->base_bn_ < N_DIRECT) {
		// TODO: journal this update
		assert(region->maxbcount_ == 1);
		// what if the region is already placed? 
		// need to compare first
		daddrs_[region->base_bn_] = region->dblock_;
	} else {
		if (region->slot_.slot_base_) {
			// TODO: journal this update
			if (region->maxbcount_ > 1) {
				region->slot_.slot_base_[region->slot_.slot_offset_] = region->radixtree_.rnode_; 
			} else {
				region->slot_.slot_base_[region->slot_.slot_offset_] = region->dblock_; 
			}
			printf("region=%p\n", region);
			printf("region->slot=%p\n", &region->slot_);
			printf("region->slot_.slot_base=%p[%d]\n", region->slot_.slot_base_, region->slot_.slot_offset_);
		} else {
			// region extends the pinode
			// TODO: journal this update
			printf("extend the pinode\n");
			printf("radixtree_.rnode_=%p\n", radixtree_.rnode_);
			if (radixtree_.rnode_) {
				// pinode's radixtree exists so we need to move it under the
				// new region's radixtree. To do so, we first need to extend
				// the pinode's radixtree to reach the height of the new
				// radixtree minus one. Then we can attach the old radixtree
				// to the new one.
				printf("radixtree_.height = %d\n", radixtree_.height_);
				printf("region->radixtree_.height = %d\n", region->radixtree_.height_);
				assert(radixtree_.height_ > 0 && 
				       region->radixtree_.height_ > radixtree_.height_);
				newheight = region->radixtree_.height_ - 1;
				printf("newheight = %d\n", newheight);
				printf("radixtree_.height = %d\n", radixtree_.height_);
				if (newheight > radixtree_.height_) {
					bcount = 1 << ((newheight)*RADIX_TREE_MAP_SHIFT);
					radixtree_.Extend(bcount-1);
				}
				RadixTreeNode* node = region->radixtree_.rnode_;
				assert(node->slots[0] == NULL);
				node->slots[0] = radixtree_.rnode_;
				radixtree_ = region->radixtree_;
			} else {
				radixtree_ = region->radixtree_;
				printf("region=%p\n", region);
				printf("region->radixtree_.rnode_=%p\n", region->radixtree_.rnode_);
				printf("radixtree_.rnode_=%p\n", radixtree_.rnode_);
			}	
		}
	}

	return 0;
}


int 
PInode::ReadBlock(uint64_t bn, char* dst, int n)
{
	int s;
	int rn;
	printf("PInode::ReadBlock(%llu, %p, %d)\n", bn, dst, n);

	s = min(BLOCK_SIZE, size_ - bn*BLOCK_SIZE);
	rn = min(n,s);

	Region region(this, bn);
	region.ReadBlock(bn, dst, rn);

	if (n - rn > 0) {
		memset(&dst[rn], 0, n-rn);
	}	
	return rn;
}


// Physically link an extent to an inode slot.
int 
PInode::PatchSlot(PInode::Slot& link)
{
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
	char           buf[4096];

	printf("PInode::WriteBlock(%llu, %p, %d)\n", bn, src, n);

	Region region(this, bn);
	printf("PInode::WriteBlock region->radixtree_.rnode_=%p\n", region.radixtree_.rnode_);
	region.WriteBlock(bn, src, n);
	printf("PInode::WriteBlock region->radixtree_.rnode_=%p\n", region.radixtree_.rnode_);
	InsertRegion(&region);

	printf("PInode::WriteBlock: DONE\n");
	if ( (bn*BLOCK_SIZE + n) > size_ ) {
		size_ = bn*BLOCK_SIZE + n;
	}
	return n;
}


int PInode::Write(char* src, uint64_t off, uint64_t n)
{
	uint64_t         tot;
	uint64_t         m;
	uint64_t         fbn; // first block number
	uint64_t         lbn; // last block number
	uint64_t         bn;
	uint64_t         fb;
	uint64_t         lb;
	uint64_t         mn;
	int              ret1;
	int              ret2;

	fb = off/BLOCK_SIZE;
	lb = (off+n)/BLOCK_SIZE;
	
/*
	for(tot=0; tot<n; tot+=m, off+=m, src+=m) {
		pinode_->LookupBlock(off/BLOCK_SIZE);
		m = min(n - tot, BLOCK_SIZE - off%BLOCK_SIZE);
		memmove(off%BSIZE, src, m);
	}
*/



}


int 
PInode::LookupSlot(uint64_t bn, Slot* slot)
{
	uint64_t       rbn;
	RadixTreeNode* node;
	int            slot_offset;
	int            slot_height;
	int            ret;
	uint64_t       bcount;
	int            i;

	if (!slot) {
		return -EINVAL;
	}
	//printf("PInode::LookupSlot (bn=%llu)\n", bn);
	return slot->Init(this, bn);
}


int 
PInode::Region::WriteBlock(uint64_t bn, char* src, int n)
{
	void**         slot;
	RadixTreeNode* node;
	int            offset;
	int            height;
	int            ret;
	uint64_t       rbn;

	printf("PInode::Region::WriteBlock(%llu, %p, %d)\n", bn, src, n);

	if (base_bn_ > bn || base_bn_+maxbcount_ <= bn) {
		// block number out of range
		return -EINVAL;
	}

	if (maxbcount_ == 1) {
		slot = &dblock_;
	} else {
		rbn = bn - base_bn_;
		// TODO: any allocations and assignments done by Mapslot must be journalled
		if ((ret = radixtree_.MapSlot(rbn, 1, &node, &offset, &height)) == 0) {
			slot = &node->slots[offset];
		} else {
			return ret;
		}
	}	
	if (*slot == (void*)0) {
		//FIXME: we should allocate a chunk instead of using malloc
		//journal allocation and assignment
		*slot = malloc(BLOCK_SIZE);
		printf("PInode::Region::WriteBlock block=%p\n", *slot);
		memset(((char*)*slot)+n, 0, BLOCK_SIZE-n); // zero the part of the block 
		                                           // that won't be written
	}
	memmove(*slot, src, n);


	return n;
}


int 
PInode::Region::ReadBlock(uint64_t bn, char* src, int n)
{
	void**         slot;
	RadixTreeNode* node;
	int            offset;
	int            height;
	int            ret;
	uint64_t       rbn;

	if (base_bn_ > bn || base_bn_+maxbcount_ <= bn) {
		// block number out of range
		return -EINVAL;
	}

	printf("PInode::Region::ReadBlock (bn=%llu)\n", bn);
	printf("PInode::Region::ReadBlock maxbcount_=%d\n", maxbcount_);

	if (maxbcount_ == 1) {
		slot = &dblock_;
	} else {
		rbn = bn - base_bn_;
		if (radixtree_.MapSlot(rbn, 0, &node, &offset, &height) == 0) {
			slot = &node->slots[offset];
			printf("node=%p, node->slots[%d]=%p\n", node, offset, node->slots[offset]);
		} else {
			slot = NULL;
		}
	}
	if (slot && *slot) {
		memmove(src, *slot, n);
	} else {
		memset(src, 0, n); 
	}
	return n;
}

/*
int PInode::Region::Write(char* src, uint64_t off, uint64_t n)
{


}
*/
