#ifndef __PINODE_H_AKE111
#define __PINODE_H_AKE111

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include "mfs/mfs_i.h"
#include "mfs/radixtree.h"
#include "common/debug.h"

const int N_DIRECT = 8;

class Extent;

class PInode 
{
public:
	PInode();
	
	int PhysicalLink(void** slot, Extent* extent);
	// LookupSlot
	// LinkBlock2Slot
	// CompareAndSwapBlock? Verifies, and Links new block to slot and returns old block
	// SwapBlock? Links new block to slot and returns old block
	uint64_t LookupBlock(uint64_t bn);
	int LookupBlockRange(uint64_t bn, uint64_t n); // this returns a pointer to array of contiguous slots
	//SwapBlock
	int InsertBlock(void*, uint64_t, int);

	uint64_t NextSlot(uint64_t bn, void*** slot, int* nslots, int offset, int height);

	// Helper functions (mostly for testing functionality)
	int ReadBlock(uint64_t bn, char* dst, int n);
	int WriteBlock(uint64_t bn, char* src, int n);

	inline uint64_t get_size() {
		return size_;
	}
	class Link;
	class Iterator;
	class Region;

private:
	uint64_t  size_;
	void*     daddrs_[N_DIRECT];
	RadixTree radixtree_;

};

class PInode::Link {
public:



//private:
	slot 
};

class PInode::Region {
public:
	Region():
		pinode_(NULL),
		base_bn_(0), 
		size_(0),
		slot_base_(NULL),
		slot_offset_(0),
		slot_height_(0)
	{ }

	Region(const PInode::Region& copy):
		pinode_(copy.pinode_),
		base_bn_(copy.base_bn_), 
		size_(copy.size_),
		slot_base_(copy.slot_base_),
		slot_offset_(copy.slot_offset_),
		slot_height_(copy.slot_height_)
	{ }

	Region(PInode* pinode, uint64_t bn)
	{
		Init(pinode, bn);
	}

	///
	/// Initializes region to represent the region containing block BN of
	/// persistent inode PINODE
	///
	int Init(PInode* pinode, uint64_t bn) 
	{
		uint64_t       rbn;
		RadixTreeNode* node;
		int            slot_offset;
		int            height;
		int            ret;
		uint64_t       size;
		int            i;

		pinode_ = pinode;

		if (bn < N_DIRECT) {
			base_bn_ = bn;
			size_ = 1;
			slot_base_ = pinode->daddrs_;
			slot_offset_ = bn;
			slot_height_ = 0;
		} else {
			rbn = bn - N_DIRECT; 
			if ( (ret = pinode->radixtree_.MapSlot(rbn, 0, &node, 
			                                       &slot_offset, &height)) != 0) 
			{
				return ret;
			}	
			for (i=1, size=1; i<height; i++) {
				size *= RADIX_TREE_MAP_SIZE;
			}
			base_bn_ = N_DIRECT + (rbn & ~(size-1));
			size_ = size;
			slot_base_ = node->slots;
			slot_offset_ = slot_offset;
			slot_height_ = height;
		}

		return 0;
	}

	PInode::Region& operator=(PInode::Region other)
	{
		pinode_ = other.pinode_;
		base_bn_ = other.base_bn_;
		slot_height_ = other.slot_height_;
		size_ = other.size_;
		slot_offset_ = other.slot_offset_;
		slot_base_ = other.slot_base_;

		return *this;
	}

	
//private:
	PInode*  pinode_;
	uint64_t base_bn_;          // zone's base block number 			
	uint64_t size_;             // zone's size in blocks
	void**   slot_base_;
	int      slot_offset_;
	int      slot_height_;
};


class PInode::Iterator {
public:
	Iterator() { }

	Iterator(PInode* pinode, uint64_t bn = 0)
	{
		assert(pinode);
		current_.Init(pinode, bn);
	}

    Iterator(const PInode::Iterator& val)
    //  	start_(val.start_), current_(val.current_) {}
	{}

    const PInode::Region current() const { 
		//return current_->data(); 
	}

	inline int NextRegion() 
	{
		RadixTreeNode* node;
		int            height;
		uint64_t       size;
		uint64_t       new_bn;
		int            ret = 0;
		
		if (current_.base_bn_ < N_DIRECT-1) {
			current_.base_bn_++;
			current_.slot_offset_++;
			current_.size_ = 1;
			assert(current_.slot_height_ == 0);
		} else if (current_.base_bn_ < N_DIRECT) {
			ret = current_.Init(current_.pinode_, current_.base_bn_+1);
		} else {
			// Check whether we stay within the same indirect block, which 
			// happens if:
			// 1) The next slot is in the same indirect block AND
			// 2) Either:
			//    i) we are in a leaf indirect block, or
			//   ii) we are in an intermediate indirect block and the next 
			//       slot does not point to another indirect block

			if ( (current_.slot_offset_+1 < RADIX_TREE_MAP_SIZE-1) /* case 1 */ &&
			     ( (current_.slot_height_ == 1)) /* case 2i */ ||
			       ((current_.slot_height_ > 1) && 
					(current_.slot_base_[current_.slot_offset_+1] == NULL)) ) /* case 2ii */ 
			{
				assert((current_.slot_height_ == 1 && current_.size_ == 1) ||
				       (current_.slot_height_ > 1));
				current_.base_bn_+=current_.size_;
				current_.slot_offset_++;
			} else {
				current_.base_bn_+=current_.size_;
				ret = current_.Init(current_.pinode_, current_.base_bn_);
			}
		}
		return ret;
	}


    void succ() { 
		NextRegion();
	}

    const int terminate() const {
      //return current_ == NULL;
    }

    void operator ++(int) {
		succ();
    }

	PInode::Iterator &operator =(const PInode::Iterator &val) {
		current_ = val.current_;
		return *this;
	}

	PInode::Region& operator *() {
		return current_;
	}


private:
	PInode::Region start_;
	PInode::Region current_;
};


class Extent {
public:
	Extent():
		ptr_(NULL),
		base_bn_(0), 
		bsize_(0)
	{ }

	Extent(const PInode::Region& zone)
	{ }

	Extent(uint64_t base_bn, uint64_t bsize)
	{
		Init(base_bn, bsize);
	}

	int Init(uint64_t base_bn, uint64_t bsize) 
	{
		RadixTree*     radixtree;
		int            ret;
		uint64_t       size;
		int            i;

		assert(bsize > 0);

		bsize_ = bsize;
		if (bsize == 1) {
			// Single block
			ptr_ = NULL;
		} else {
			// Multiple blocks: use a radix tree
			// We don't need the root of the radix tree to be persistent, just
			// the indirect blocks (which is already handled by the radix tree
			// code).
			base_bn_ = base_bn;
			radixtree = new RadixTree;
			ptr_ = (void*) radixtree;
			radixtree->Extend(bsize-1);
		}

		return 0;
	}

	Extent& operator=(Extent other)
	{
		ptr_ = other.ptr_;
		base_bn_ = other.base_bn_;
		bsize_ = other.bsize_;

		return *this;
	}

	int WriteBlock(uint64_t rbn, char* src, int n);

//private:
	void*    ptr_;	     // Points to a single direct block if bsize_ == 1 or 
	                     // a radix tree if bsize_ > 1
	uint64_t base_bn_;   // Extent's base block number 			
	uint64_t bsize_;     // Extent's size in blocks
};


int 
Extent::WriteBlock(uint64_t rbn, char* src, int n)
{
	void**         slot;
	RadixTreeNode* node;
	RadixTree*     radixtree;
	int            offset;
	int            height;
	int            ret;

	if (bsize == 1) {
		slot = &ptr_;
	} else {
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

	return n;
}


#endif // __PINODE_H_AKE111
