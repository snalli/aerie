#include "mfs/pinode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common/errno.h"

#define min(a,b) ((a) < (b)? (a) : (b))


//FIXME: rename PInode to FilePnode

inline uint64_t 
block_valid_data(int bn, uint64_t file_size)
{
	return ((((bn + 1) * BLOCK_SIZE) < file_size ) 
	        ? BLOCK_SIZE 
	        : (file_size - (bn * BLOCK_SIZE)));
}


template<typename T>
int __Write(T* obj, char* src, uint64_t off, uint64_t n)
{
	uint64_t tot;
	uint64_t m;
	uint64_t bn;
	uint64_t f;
	int      ret;

	for(tot=0; tot<n; tot+=m, off+=m) {
		bn = off / BLOCK_SIZE;
		f = off % BLOCK_SIZE;
		m = min(n - tot, BLOCK_SIZE - f);
		ret = obj->WriteBlock(&src[tot], bn, f, m);
		if (ret < m) {
			return ((ret < 0) ? ( (tot>0)? tot: ret)  
			                  : tot + ret);
		}
	}

	return tot;
}


template<typename T>
int __Read(T* obj, char* dst, uint64_t off, uint64_t n)
{
	uint64_t tot;
	uint64_t bn;
	int      m;
	int      f;
	int      ret;

	for(tot=0; tot<n; tot+=m, off+=m) {
		bn = off / BLOCK_SIZE;
		f = off % BLOCK_SIZE;
		m = min(n - tot, BLOCK_SIZE - f);
		ret = obj->ReadBlock(&dst[tot], bn, f, m);
		if (ret < 0) {
			return ((ret < 0) ? ( (tot>0)? tot: ret)  
			                  : tot + ret);
		}
	}

	return tot;
}



/////////////////////////////////////////////////////////////////////////////
// 
// PInode
//
/////////////////////////////////////////////////////////////////////////////



PInode::PInode()
{
	for (int i=0; i<N_DIRECT; i++) {
		daddrs_[i] = (void*)0;
	}
}


// trusts the callee has already check that bn is out of range
int 
PInode::Region::Extend(uint64_t bn)
{
	// can only extend a region if the region is not allocated to an 
	// existing slot
	if (slot_.slot_base_) {
		return -1;
	}
	radixtree_.Extend(bn - base_bn_);
	maxbcount_ = 1 << ((radixtree_.height_)*RADIX_TREE_MAP_SHIFT);
	return 0;
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
			// no slot, therefore region extends the pinode
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


// Physically link an extent to an inode slot.
int 
PInode::PatchSlot(PInode::Slot& link)
{
	return 0;
}


// FIXME:
// The region object is destroyed when this function returns. What happens 
// to the persistent storage it points to??? For example, what would happen
// if someone else destroys the file??? We should still be able to 
// reference the blocks
// Perhaps we should use some refcounting
int 
PInode::ReadBlock(char* dst, uint64_t bn, int off, int n)
{
	int l;
	int rn;

	printf("PInode::ReadBlock(dst=%p, bn=%llu, off=%d, n=%d)\n", dst, bn, off, n);

	l = min(BLOCK_SIZE, size_ - bn*BLOCK_SIZE);
	if (l < off) {
		return 0;
	}
	rn = min(n,l-off);

	Region region(this, bn);
	region.ReadBlock(dst, bn, off, rn);

	if (n - rn > 0) {
		memset(&dst[rn], 0, n-rn);
	}	
	return rn;
}


// FIXME: do we need to dynamically allocate region instead to keep it around 
// until we publish it???
// The region object is destroyed when this function returns. What happens 
// to the persistent storage it points to???
// Perhaps we should use some refcounting
int 
PInode::WriteBlock(char* src, uint64_t bn, int off, int n)
{
	uint64_t       rbn;
	void**         slot;
	RadixTreeNode* node;
	int            offset;
	int            height;
	int            ret;
	char           buf[4096];

	printf("PInode::WriteBlock(src=%p, bn=%llu, off=%d, n=%d)\n", src, bn, off, n);

	Region region(this, bn);
	printf("PInode::WriteBlock region->radixtree_.rnode_=%p\n", region.radixtree_.rnode_);
	if ( (ret = region.WriteBlock(src, bn, off, n)) < 0) {
		return ret;
	}
	
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
	return __Write<PInode> (this, src, off, n);
}


int PInode::Read(char* dst, uint64_t off, uint64_t n)
{
	return __Read<PInode> (this, dst, off, n);
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
		return -E_INVAL;
	}
	//printf("PInode::LookupSlot (bn=%llu)\n", bn);
	return slot->Init(this, bn);
}


/////////////////////////////////////////////////////////////////////////////
// 
// PInode::Region
//
/////////////////////////////////////////////////////////////////////////////



// off is offset with respect to the block
int 
PInode::Region::WriteBlock(char* src, uint64_t bn, int off, int n)
{
	void**         slot;
	char*          bp;
	RadixTreeNode* node;
	int            offset;
	int            height;
	int            ret;
	uint64_t       rbn;

	assert(off < BLOCK_SIZE);
	assert(off+n <= BLOCK_SIZE);

	printf("PInode::Region::WriteBlock(src=%p, bn=%llu, off=%d, n=%d)\n", src, bn, off, n);

	// check if block number out of range
	if (base_bn_ > bn) {
		return -E_INVAL;
	} else if (base_bn_+maxbcount_ <= bn) {
		if (Extend(bn) < 0) {
			// block out of range and cannot extend the region
			return -E_INVAL;
		}
	}

	if (maxbcount_ == 1) {
		slot = &dblock_;
	} else {
		rbn = bn - base_bn_;
		// TODO: any allocations and assignments done by Mapslot must be journalled
		if ((ret = radixtree_.MapSlot(rbn, 1, 1, &node, &offset, &height)) == 0) {
			slot = &node->slots[offset];
		} else {
			return ret;
		}
	}	
	if (*slot == (void*)0) {
		//FIXME: we should allocate a chunk instead of using malloc
		//journal allocation and assignment
		*slot = malloc(BLOCK_SIZE);
		bp = (char*) (*slot);
		printf("PInode::Region::WriteBlock block=%p\n", *slot);
		// TODO: Allocating and zeroing a chunk is done in other places in the 
		// code as well. We should collapse this under a function that does the job
		// (including any necessary journaling?)
		// Zero the part of the newly allocated block that is not written to
		// ensure we later read zeros and not garbage.
		if (off>0) {
			memset(bp, 0, off);
		}	
		memset(&bp[off+n], 0, BLOCK_SIZE-n); 
	}
	bp = (char*) (*slot);
	memmove(&bp[off], src, n);

	return n;
}


int PInode::Region::Write(char* src, uint64_t off, uint64_t n)
{
	printf("PInode::Region::Write(src=%p, off=%llu, n=%llu)\n", src, off, n);

	return __Write<PInode::Region> (this, src, off, n);
}


int 
PInode::Region::ReadBlock(char* dst, uint64_t bn, int off, int n)
{
	void**         slot;
	char*          bp;
	RadixTreeNode* node;
	int            offset;
	int            height;
	int            ret;
	uint64_t       rbn;

	assert(off < BLOCK_SIZE);
	assert(off+n <= BLOCK_SIZE);

	printf("PInode::Region::ReadBlock(dst=%p, bn=%llu, off=%d, n=%d)\n", dst, bn, off, n);

	if (base_bn_ > bn || base_bn_+maxbcount_ <= bn) {
		// block number out of range
		return -E_INVAL;
	}

	printf("PInode::Region::ReadBlock maxbcount_=%d\n", maxbcount_);

	if (maxbcount_ == 1) {
		slot = &dblock_;
	} else {
		rbn = bn - base_bn_;
		if (radixtree_.MapSlot(rbn, 1, 0, &node, &offset, &height) == 0) {
			slot = &node->slots[offset];
			printf("node=%p, node->slots[%d]=%p\n", node, offset, node->slots[offset]);
		} else {
			slot = NULL;
		}
	}
	if (slot && *slot) {
		bp = (char*) (*slot);
		memmove(dst, &bp[off], n);
	} else {
		memset(dst, 0, n); 
	}
	return n;
}


int PInode::Region::Read(char* dst, uint64_t off, uint64_t n)
{
	return __Read<PInode::Region> (this, dst, off, n);
}
