#ifndef __PINODE_H_AKE111
#define __PINODE_H_AKE111

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include "mfs/mfs_i.h"
#include "mfs/radixtree.h"
#include "common/debug.h"

const int N_DIRECT = 8;

// A PInode is logically composed of non-overlapping regions
// A PInode is physically implemented using a radixtree that connects 
// the logical regions which are physically implemented as subtrees 
// or leafs (direct blocks) of the radix tree.


// FIXME: Currently PInode is a wrapper around the persistent inode
// so it does not keep any volatile state. However PInode::Region
// and PInode::Slot have volatile state. It seems that this confuses
// as what part of the PInode class is persistent and what is not
// Consider reformatting the PInode to be a volatile object that 
// has a pointer to the persistent state. In this case PInode 
// can contain volatile state which could be helpful to higher 
// layers, such as Inode. Inode for example has to keep information
// whether the PInode is mutable or not, or whether the PInode 
// has been published or not. Also the Inode has to keep a journal 
// that passed to the PInode whenever performing operations
// It seems all this state should belong to the PInode. 


class PInode 
{
public:
	class Slot;
	class Iterator;
	class Region;

	PInode();
	
	// LookupSlot
	// SlotBlock2Slot
	// CompareAndSwapBlock? Verifies, and Slots new block to slot and returns old block
	// SwapBlock? Slots new block to slot and returns old block
	int LookupSlot(uint64_t, Slot*);
	//SwapBlock
	int PatchSlot(PInode::Slot& link);
	int InsertRegion(Region* region);
	int AllocateRegion(uint64_t start_bn, uint64_t end_bn);

	int Write(char*, uint64_t, uint64_t);
	int Read(char*, uint64_t, uint64_t);

	int ReadBlock(char*, uint64_t, int, int);
	int WriteBlock(char*, uint64_t, int, int);

	// return maximum possible size in bytes
	inline uint64_t get_maxsize() { return get_maxbcount() * BLOCK_SIZE; }

	// return maximum possible size in bytes
	inline uint64_t get_maxbcount() {
		uint64_t nblocks;

		nblocks = (1 << ((radixtree_.height_)*RADIX_TREE_MAP_SHIFT));
		nblocks += N_DIRECT;

		return nblocks;
	}

	inline uint64_t get_size() { return size_; }

private:
	uint64_t  size_;
	void*     daddrs_[N_DIRECT];
	RadixTree radixtree_;
};


class PInode::Slot {
public:

	Slot()
		: pinode_(NULL),
		  slot_base_(NULL)
	{ }

	Slot(const PInode::Slot& copy)
		: pinode_(copy.pinode_),
		  slot_base_(copy.slot_base_),
		  slot_offset_(copy.slot_offset_),
		  slot_height_(copy.slot_height_),
		  base_bn_(copy.base_bn_)
	{ }	



	Slot(PInode* pinode, uint64_t base_bn)
	{
		Init(pinode, base_bn);
	}


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


	int Init(PInode* const pinode, uint64_t bn)
	{
		uint64_t       rbn;
		RadixTreeNode* node;
		int            slot_offset;
		int            slot_height;
		int            ret;
		uint64_t       bcount;
		int            i;

		pinode_ = pinode;
		if (bn < N_DIRECT) {
			base_bn_ = bn;
			slot_base_ = pinode->daddrs_;
			slot_offset_ = bn;
			slot_height_ = 1;
		} else {
			rbn = bn - N_DIRECT; 
			if ( (ret = pinode->radixtree_.MapSlot(rbn, 0, &node, 
			                                       &slot_offset, 
			                                       &slot_height)) != 0) 
			{
				slot_base_ = NULL;
				return ret;
			} else {
				bcount = 1 << ((slot_height-1)*RADIX_TREE_MAP_SHIFT);
				base_bn_ = N_DIRECT + (rbn & ~(bcount-1));
				slot_base_ = (void**) node;
				slot_offset_ = slot_offset;
				slot_height_ = slot_height;
			}
		}

		return 0;
	}


	PInode::Slot& operator=(const PInode::Slot& other)
	{
		pinode_ = other.pinode_;
		slot_base_ = other.slot_base_;
		slot_offset_ = other.slot_offset_;
		slot_height_ = other.slot_height_;
		base_bn_ = other.base_bn_;

		return *this;
	}

	uint64_t get_base_bn()
	{
		return base_bn_;
	}

//private:
	// physical information
	PInode*        pinode_;
	void**         slot_base_;
	int            slot_offset_;
	int            slot_height_;
	// logical information
	uint64_t       base_bn_;     // The block number of the first block indexed 
	                             //  at this slot
};


// Region is logically a subregion of PInode 
// offsets and block numbers passed as arguments are relative to 
// the start of PInode and not to the start of the region.

class PInode::Region {
public:
	Region()
		: maxbcount_(0),
		  base_bn_(0),
		  size_(0)
	{ }

	Region(const PInode::Region& copy)
	{ }

	Region(PInode* pinode, uint64_t base_bn) 
	{
		assert(Init(pinode, base_bn) == 0);
	}

	Region(Slot& slot)
		: slot_(slot)
	{
		assert(InitAtSlot() == 0);
	}

	Region(uint64_t base_bn, uint64_t maxbcount)
	{
		maxbcount_ = maxbcount;
		base_bn_ = base_bn;
		size_ = 0;
		if (maxbcount > 1) {
			radixtree_.rnode_ = NULL;
			radixtree_.Extend(maxbcount_-1);
		} else {
			dblock_ = NULL;
		}
	}


	///
	/// Initializes region to represent the region containing block BN of
	/// persistent inode PINODE
	///
	int Init(PInode* pinode, uint64_t bn) 
	{
		slot_.Init(pinode, bn);

		if (slot_.slot_base_ == NULL) {
			// No slot. Create an orphan region larger than pinode.
			printf("Region::Init: no slot\n");
			radixtree_.rnode_ = NULL;
			radixtree_.Extend(bn - N_DIRECT);
			printf("Region::Init: height = %d\n", radixtree_.height_);
			maxbcount_ = 1 << ((radixtree_.height_)*RADIX_TREE_MAP_SHIFT);
			base_bn_ = N_DIRECT;
			return 0;
		}	

		return InitAtSlot();
	}


	///
	/// Initializes region to represent the region rooted at slot_
	///
	int InitAtSlot() 
	{
		uint64_t bcount;
		int      ret;

		if (!slot_.slot_base_) {
			return -1;
		}

		base_bn_ = slot_.base_bn_;
		if (base_bn_ < N_DIRECT) {
			maxbcount_ = 1;
			dblock_ = slot_.slot_base_[slot_.slot_offset_];
			//printf("slot_.slot_base_[slot_.slot_offset_]=%p\n", slot_.slot_base_[slot_.slot_offset_]);
			//printf("pinode->daddrs_[bn]=%p\n", pinode->daddrs_[bn]);
		} else {
			maxbcount_ = 1 << ((slot_.slot_height_-1)*RADIX_TREE_MAP_SHIFT);
			printf("Region::Init: slot_base=%p\n", slot_.slot_base_);
			printf("Region::Init: slot_offset=%d\n", slot_.slot_offset_);
			printf("Region::Init: slot_height=%d\n", slot_.slot_height_);
			if (bcount > 1) {
				assert(slot_.slot_base_[slot_.slot_offset_] == NULL);
				radixtree_.rnode_ = NULL;
				radixtree_.Extend(maxbcount_-1);
			} else {
				dblock_ = slot_.slot_base_[slot_.slot_offset_];
			}
		}

		return 0;
	}


	// shallow copy (does not make a deep copy of radix tree)
	PInode::Region& operator=(const PInode::Region& other)
	{
		slot_ = other.slot_;
		base_bn_ = other.base_bn_;
		maxbcount_ = other.maxbcount_;
		size_ = other.size_;
		if (maxbcount_ > 1) {
			radixtree_ = other.radixtree_;
		} else {
			dblock_ = other.dblock_;
		}

		return *this;
	}

	int WriteBlock(char*, uint64_t, int, int);
	int ReadBlock(char*, uint64_t, int, int);

	int Write(char*, uint64_t, uint64_t);
	int Read(char*, uint64_t, uint64_t);

	int Extend(uint64_t bn);

//private:
	Slot      slot_;       // Region's physical slot in the inode
	uint64_t  base_bn_;    // Region's base block number 			
	uint64_t  maxbcount_;  // Region's max allowed size in blocks
	uint64_t  size_;       // Region's size in bytes (NOT YET USED)
	void*     dblock_;     // Points to a single direct block if maxbcount_ == 1 
	RadixTree radixtree_;  // A valid radix tree if maxbcount_ > 1
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

	inline int NextSlot() 
	{
		RadixTreeNode* node;
		int            height;
		uint64_t       size;
		uint64_t       new_bn;
		int            ret = 0;
		uint64_t       bcount;
		
		if (current_.base_bn_ < N_DIRECT-1) {
			current_.slot_offset_++;
			current_.base_bn_++;
			assert(current_.slot_height_ == 1);
		} else if (current_.base_bn_ < N_DIRECT) {
			ret = current_.pinode_->LookupSlot(current_.base_bn_+1, &current_); 
		} else {
			// Check whether we stay within the same indirect block, which 
			// happens if:
			// 1) The next slot is in the same indirect block AND
			// 2) Either:
			//    i) we are in a leaf indirect block, or
			//   ii) we are in an intermediate indirect block and the next 
			//       slot does not point to another indirect block
			assert(current_.slot_height_ > 0);
			bcount = 1 << ((current_.slot_height_-1)*RADIX_TREE_MAP_SHIFT);
			if ( (current_.slot_offset_+1 < RADIX_TREE_MAP_SIZE-1) /* case 1 */ &&
			     ( (current_.slot_height_ == 1)) /* case 2i */ ||
			       ((current_.slot_height_ > 1) && 
					(current_.slot_base_[current_.slot_offset_+1] == NULL)) ) /* case 2ii */ 
			{
				current_.base_bn_+=bcount;
				current_.slot_offset_++;
			} else {
				current_.base_bn_+=bcount;
				ret = current_.pinode_->LookupSlot(current_.base_bn_, &current_);
			}
		}
		return ret;
	}

    void succ() { 
		NextSlot();
	}

    const int terminate() const {
    	return current_.slot_base_ == NULL;
    }

    void operator ++(int) {
		succ();
    }

	PInode::Iterator &operator =(const PInode::Iterator &val) {
		current_ = val.current_;
		return *this;
	}

	PInode::Slot& operator *() {
		return current_;
	}

private:
	PInode::Slot current_;
};

#endif // __PINODE_H_AKE111
