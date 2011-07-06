#ifndef __PINODE_H_AKE111
#define __PINODE_H_AKE111

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include "mfs/mfs_i.h"
#include "mfs/radixtree.h"
#include "common/debug.h"

const int N_DIRECT = 8;

class PInode 
{
public:
	class Link;
	class Iterator;
	class Region;

	PInode();
	
	int PhysicalLink(void**, Region*);
	// LookupSlot
	// LinkBlock2Slot
	// CompareAndSwapBlock? Verifies, and Links new block to slot and returns old block
	// SwapBlock? Links new block to slot and returns old block
	int LookupBlock(uint64_t bn, Link* link, Region* region);
	//int LookupBlockRange(uint64_t bn, uint64_t n); // this returns a pointer to array of contiguous slots
	//SwapBlock
	int PatchLink(PInode::Link& link);
	int InsertBlock(void*, uint64_t, int);
	int InsertRegion(uint64_t bn, Region* region);
	int AllocateRegion(uint64_t start_bn, uint64_t end_bn);

	uint64_t NextSlot(uint64_t bn, void*** slot, int* nslots, int offset, int height);


	int Write(char*, uint64_t, uint64_t);

	// Helper functions (mostly for testing functionality)
	int ReadBlock(uint64_t bn, char* dst, int n);
	int WriteBlock(uint64_t bn, char* src, int n);

	inline uint64_t get_maxsize() {
		uint64_t nblocks;

		nblocks = (1 << ((radixtree_.height_)*RADIX_TREE_MAP_SHIFT));
		nblocks += N_DIRECT;

		return nblocks * BLOCK_SIZE;
	}

	inline uint64_t get_size() {
		return size_;
	}

private:
	uint64_t  size_;
	void*     daddrs_[N_DIRECT];
	RadixTree radixtree_;

};


class PInode::Region {
public:
	Region():
		base_bn_(0), 
		bsize_(0)
	{ }

	Region(const PInode::Region& copy)
	{ }

	Region(uint64_t base_bn, uint64_t bsize)
	{
		Init(NULL, base_bn, bsize);
	}

	int Init(void* ptr, uint64_t base_bn, uint64_t bsize) 
	{
		int      ret;
		uint64_t size;
		int      i;

		assert(bsize > 0);

		if (bsize == 1) {
			dblock_ = ptr;
		} else {
			//FIXME: if ptr != NULL, do we make a deep copy?
			// base_bn must be integer multiple of bsize
			radixtree_.rnode_=NULL;
			radixtree_.Extend(bsize-1);
			radixtree_.rnode_ = reinterpret_cast<RadixTreeNode*>(ptr);
		}
		base_bn_ = base_bn;
		bsize_ = bsize;

		return 0;
	}

	///
	/// Initializes region to represent the region containing block BN of
	/// persistent inode PINODE
	///
	int Init(PInode* pinode, uint64_t bn) 
	{
		return 0;
	}

	// shallow copy (does not make a deep copy of radix tree)
	PInode::Region& operator=(const PInode::Region& other)
	{
		base_bn_ = other.base_bn_;
		bsize_ = other.bsize_;
		if (bsize_ > 1) {
			radixtree_ = other.radixtree_;
		} else {
			dblock_ = other.dblock_;
		}

		return *this;
	}

	int WriteBlock(uint64_t rbn, char* src, int n);
	int ReadBlock(uint64_t rbn, char* src, int n);

//private:
	uint64_t  base_bn_;    // Region's base block number 			
	uint64_t  bsize_;      // Region's size in blocks
	void*     dblock_;     // Points to a single direct block if bsize_ == 1 
	RadixTree radixtree_;  // A radix tree if bsize_ > 1
};


class PInode::Link {
public:

	int Init(PInode* pinode, 
	         void** slot_base, 
	         int slot_offset, 
	         int slot_height)
	{
		pinode_ = pinode;
		slot_base_ = slot_base;
		slot_offset_ = slot_offset;
		slot_height_ = slot_height;

		return 0;
	}

	int Init(PInode* pinode, uint64_t bn)
	{
		int ret;

		ret = pinode->LookupBlock(bn, this, &region_);
		assert(ret == 0);
	}

	PInode::Link& operator=(const PInode::Link& other)
	{
		pinode_ = other.pinode_;
		slot_base_ = other.slot_base_;
		slot_offset_ = other.slot_offset_;
		slot_height_ = other.slot_height_;
		region_ = other.region_;

		return *this;
	}


//private:
	// physical link information
	PInode*        pinode_;
	void**         slot_base_;
	int            slot_offset_;
	int            slot_height_;
	PInode::Region region_;
};


class PInode::Iterator {
public:
	Iterator() { }

	Iterator(PInode* pinode, uint64_t bn = 0)
	{
		Init(pinode, bn);
	}

    Iterator(const PInode::Iterator& val)
    //  	start_(val.start_), current_(val.current_) {}
	{}

	int Init(PInode* pinode, uint64_t bn = 0)
	{
		assert(pinode);
		current_.Init(pinode, bn);
	}

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
		
		if (current_.region_.base_bn_ < N_DIRECT-1) {
			current_.slot_offset_++;
			current_.region_.base_bn_++;
			current_.region_.bsize_ = 1;
			assert(current_.slot_height_ == 0);
		} else if (current_.region_.base_bn_ < N_DIRECT) {
			ret = current_.pinode_->LookupBlock(current_.region_.base_bn_+1, 
			                                    &current_, 
			                                    &current_.region_);
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
				assert((current_.slot_height_ == 1 && current_.region_.bsize_ == 1) ||
				       (current_.slot_height_ > 1));
				current_.region_.base_bn_+=current_.region_.bsize_;
				current_.slot_offset_++;
			} else {
				current_.region_.base_bn_+=current_.region_.bsize_;
				ret = current_.pinode_->LookupBlock(current_.region_.base_bn_+1, 
				                                    &current_, 
				                                    &current_.region_);
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

	PInode::Link& operator *() {
		return current_;
	}

private:
	//PInode::Region start_;
	PInode::Link current_;
};



#endif // __PINODE_H_AKE111
