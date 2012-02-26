//! \file
//! Definition of the name collection persistent object stored in SCM
//!

// TODO: any allocations and assignments done by Mapslot must be journalled

#ifndef __STAMNOS_DPO_COMMON_BYTE_CONTAINER_OBJECT_H
#define __STAMNOS_DPO_COMMON_BYTE_CONTAINER_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "dpo/containers/radix/radixtree.h"
#include "dpo/containers/typeid.h"
#include "dpo/main/common/obj.h"
#include "dpo/main/client/salloc.h"
#include "dpo/main/common/const.h"
#include "common/debug.h"
#include "common/util.h"

namespace dpo {
namespace containers {
namespace common {

class ByteContainer {
public:
enum {
	N_DIRECT = 8
};

template<typename Session>
class Slot;

template<typename Session>
class Iterator;

template<typename Session>
class Region;

template<typename Session>
class Object: public dpo::cc::common::Object {
public:
	static Object* Make(Session* session) {
		void* ptr;
		
		if (session->salloc()->AllocateRaw(session, sizeof(Object), &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return new(ptr) Object();
	}

	static Object* Make(Session* session, volatile char* ptr) {
		return new(ptr) Object();
	}

	static Object* Load(dpo::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}
	
	Object();

	int InsertRegion(Session* session, Region<Session>* region);
	int AllocateRegion(Session* session, uint64_t start_bn, uint64_t end_bn);

	int Write(Session* session, char* src, uint64_t off, uint64_t n);
	int Read(Session* session, char* dst, uint64_t off, uint64_t n);

	int ReadBlock(Session* session, char* dst, uint64_t bn, int off, int n);
	int WriteBlock(Session* session, char* src, uint64_t bn, int off, int n);

	// return maximum possible size in bytes
	inline uint64_t get_maxsize() { 
		return get_maxbcount() * dpo::common::BLOCK_SIZE; 
	}

	// return maximum possible size in bytes
	inline uint64_t get_maxbcount() {
		uint64_t nblocks;

		nblocks = (1 << ((radixtree_.height_)*RADIX_TREE_MAP_SHIFT));
		nblocks += N_DIRECT;

		return nblocks;
	}

	uint64_t Size() { return size_; }

	int LookupSlot(Session* session, uint64_t bn, Slot<Session>* slot);
	int PatchSlot(Slot<Session>& link);


	RadixTree<Session>* radixtree() {
		return &radixtree_;
	}

	uint64_t           size_;
	void*              daddrs_[N_DIRECT];
	RadixTree<Session> radixtree_;
}; // Object


template<typename Session>
class Slot {
public:

	Slot()
		: bcobj_(NULL),
		  slot_base_(NULL)
	{ }

	Slot(const Slot<Session>& copy)
		: bcobj_(copy.bcobj_),
		  slot_base_(copy.slot_base_),
		  slot_offset_(copy.slot_offset_),
		  slot_height_(copy.slot_height_),
		  base_bn_(copy.base_bn_)
	{ }	


	Slot(Session* session, Object<Session>* bcobj, uint64_t base_bn)
	{
		Init(session, bcobj, base_bn);
	}


	int Init(Session* session,
	         Object<Session>* bcobj, 
	         void** slot_base, 
	         int slot_offset, 
	         int slot_height)
	{
		bcobj_ = bcobj;
		slot_base_ = slot_base;
		slot_offset_ = slot_offset;
		slot_height_ = slot_height;

		return 0;
	}


	int Init(Session* session, Object<Session>* const bcobj, uint64_t bn)
	{
		uint64_t                rbn;
		RadixTreeNode<Session>* node;
		int                     slot_offset;
		int                     slot_height;
		int                     ret;
		uint64_t                bcount;
		int                     i;

		bcobj_ = bcobj;
		if (bn < N_DIRECT) {
			base_bn_ = bn;
			slot_base_ = bcobj->daddrs_;
			slot_offset_ = bn;
			slot_height_ = 1;
		} else {
			rbn = bn - N_DIRECT; 
			if ( (ret = bcobj->radixtree_.MapSlot(session, rbn, 1, 0, &node, 
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

	Slot<Session>& operator=(const Slot<Session>& other)
	{
		bcobj_ = other.bcobj_;
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

	// physical information
	Object<Session>* bcobj_;
	void**           slot_base_;
	int              slot_offset_;
	int              slot_height_;
	// logical information
	uint64_t         base_bn_;     // The block number of the first block indexed 
	                               //  at this slot
}; // Slot


// Region is logically a subregion of FilePnode 
// offsets and block numbers passed as arguments are relative to 
// the start of FilePnode and not to the start of the region.
template<typename Session>
class Region {
public:
	Region()
		: maxbcount_(0),
		  base_bn_(0),
		  size_(0)
	{ }

	Region(const Region& copy)
	{ }

	Region(Session* session, Object<Session>* bcobj, uint64_t base_bn) 
	{
		assert(Init(session, bcobj, base_bn) == 0);
	}

	Region(Session* session, Slot<Session>& slot)
		: slot_(slot)
	{
		assert(InitAtSlot(session) == 0);
	}

	Region(Session* session, uint64_t base_bn, uint64_t maxbcount)
	{
		maxbcount_ = maxbcount;
		base_bn_ = base_bn;
		size_ = 0;
		if (maxbcount > 1) {
			radixtree_.rnode_ = NULL;
			radixtree_.Extend(session, maxbcount_-1);
		} else {
			dblock_ = NULL;
		}
	}


	///
	/// Initializes region to represent the region containing block BN of
	/// persistent byte container bcobj
	///
	int Init(Session* session, Object<Session>* bcobj, uint64_t bn) 
	{
		slot_.Init(session, bcobj, bn);

		if (slot_.slot_base_ == NULL) {
			// No slot. Create an orphan region larger than bcobj.
			printf("Region::Init: no slot\n");
			radixtree_.rnode_ = NULL;
			radixtree_.Extend(session, bn - N_DIRECT);
			printf("Region::Init: height = %d\n", radixtree_.height_);
			maxbcount_ = 1 << ((radixtree_.height_)*RADIX_TREE_MAP_SHIFT);
			base_bn_ = N_DIRECT;
			return 0;
		}	

		return InitAtSlot(session);
	}


	///
	/// Initializes region to represent the region rooted at slot_
	///
	int InitAtSlot(Session* session) 
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
			printf("dblock=%p\n", dblock_);
			//printf("Region::Init: slot_.slot_base_[slot_.slot_offset_]=%p\n", slot_.slot_base_[slot_.slot_offset_]);
			//printf("bcobj->daddrs_[bn]=%p\n", bcobj->daddrs_[bn]);
		} else {
			maxbcount_ = 1 << ((slot_.slot_height_-1)*RADIX_TREE_MAP_SHIFT);
			printf("Region::Init: slot_base=%p\n", slot_.slot_base_);
			printf("Region::Init: slot_offset=%d\n", slot_.slot_offset_);
			printf("Region::Init: slot_height=%d\n", slot_.slot_height_);
			if (maxbcount_ > 1) {
				assert(slot_.slot_base_[slot_.slot_offset_] == NULL);
				radixtree_.rnode_ = NULL;
				radixtree_.Extend(session, maxbcount_-1);
			} else {
				dblock_ = slot_.slot_base_[slot_.slot_offset_];
			}
		}

		return 0;
	}


	// shallow copy (does not make a deep copy of radix tree)
	Region<Session>& operator=(const Region<Session>& other)
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

	int WriteBlock(Session* session, char*, uint64_t, int, int);
	int ReadBlock(Session* session, char*, uint64_t, int, int);

	int Write(Session* session, char*, uint64_t, uint64_t);
	int Read(Session* session, char*, uint64_t, uint64_t);

	int Extend(Session* session, uint64_t bn);

//private:
	Slot<Session>      slot_;       // Region's physical slot in the inode
	uint64_t           base_bn_;    // Region's base block number 			
	uint64_t           maxbcount_;  // Region's max allowed size in blocks
	uint64_t           size_;       // Region's size in bytes (NOT YET USED)
	void*              dblock_;     // Points to a single direct block if maxbcount_ == 1 
	RadixTree<Session> radixtree_;  // A valid radix tree if maxbcount_ > 1
}; // Region


template<typename Session>
class Iterator {
public:
	Iterator() { }

	Iterator(Session* session, Object<Session>* bcobj, uint64_t bn = 0)
		: session_(session)
	{
		Init(session, bcobj, bn);
	}

    Iterator(const Iterator<Session>& val)
    //  	start_(val.start_), current_(val.current_) {}
	{}

	int Init(Session* session, Object<Session>* bcobj, uint64_t bn = 0)
	{
		assert(bcobj);
		current_.Init(session, bcobj, bn);
	}

	inline int NextSlot(Session* session) 
	{
		RadixTreeNode<Session>* node;
		int                     height;
		uint64_t                size;
		uint64_t                new_bn;
		int                     ret = 0;
		uint64_t                bcount;
		
		if (current_.base_bn_ < N_DIRECT-1) {
			current_.slot_offset_++;
			current_.base_bn_++;
			assert(current_.slot_height_ == 1);
		} else if (current_.base_bn_ < N_DIRECT) {
			ret = current_.bcobj_->LookupSlot(session, current_.base_bn_+1, &current_); 
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
			     ( (current_.slot_height_ == 1) /* case 2i */ ||
			       ((current_.slot_height_ > 1) && 
					(current_.slot_base_[current_.slot_offset_+1] == NULL))) ) /* case 2ii */ 
			{
				current_.base_bn_+=bcount;
				current_.slot_offset_++;
			} else {
				current_.base_bn_+=bcount;
				ret = current_.bcobj_->LookupSlot(session, current_.base_bn_, &current_);
			}
		}
		return ret;
	}

    void succ(Session* session) { 
		NextSlot(session);
	}

    const int terminate() const {
    	return current_.slot_base_ == NULL;
    }

    void operator ++(int) {
		succ(session_);
    }

	Iterator<Session> &operator =(const Iterator<Session>& val) {
		current_ = val.current_;
		return *this;
	}

	Slot<Session>& operator *() {
		return current_;
	}

private:
	Slot<Session> current_;
	Session*      session_;
}; // Iterator



}; // ByteContainer


inline uint64_t min(uint64_t a, uint64_t b) {
	return ((a) < (b)? (a) : (b));
}


inline uint64_t 
block_valid_data(int bn, uint64_t file_size)
{
	return ((((bn + 1) * dpo::common::BLOCK_SIZE) < file_size ) 
	        ? dpo::common::BLOCK_SIZE 
	        : (file_size - (bn * dpo::common::BLOCK_SIZE)));
}


template<typename Session, typename T>
int __Write(Session* session, T* obj, char* src, uint64_t off, uint64_t n)
{
	uint64_t tot;
	uint64_t m;
	uint64_t bn;
	uint64_t f;
	int      ret;

	for(tot=0; tot<n; tot+=m, off+=m) {
		bn = off / dpo::common::BLOCK_SIZE;
		f = off % dpo::common::BLOCK_SIZE;
		m = min(n - tot, dpo::common::BLOCK_SIZE - f);
		ret = obj->WriteBlock(session, &src[tot], bn, f, m);
		if (ret < m) {
			return ((ret < 0) ? ( (tot>0)? tot: ret)  
			                  : tot + ret);
		}
	}

	return tot;
}


template<typename Session, typename T>
int __Read(Session* session, T* obj, char* dst, uint64_t off, uint64_t n)
{
	uint64_t tot;
	uint64_t bn;
	int      m;
	int      f;
	int      ret;

	for(tot=0; tot<n; tot+=m, off+=m) {
		bn = off / dpo::common::BLOCK_SIZE;
		f = off % dpo::common::BLOCK_SIZE;
		m = min(n - tot, dpo::common::BLOCK_SIZE - f);
		ret = obj->ReadBlock(session, &dst[tot], bn, f, m);
		if (ret < 0) {
			return ((ret < 0) ? ( (tot>0)? tot: ret)  
			                  : tot + ret);
		}
	}

	return tot;
}



/////////////////////////////////////////////////////////////////////////////
// 
// ByteContainer::Object
//
/////////////////////////////////////////////////////////////////////////////

template<typename Session>
ByteContainer::Object<Session>::Object()
	: size_(0)
{
	for (int i=0; i<N_DIRECT; i++) {
		daddrs_[i] = (void*)0;
	}
	set_type(T_BYTE_CONTAINER);
}


template<typename Session>
int 
ByteContainer::Object<Session>::InsertRegion(Session* session, 
                                             ByteContainer::Region<Session>* region)
{
	uint64_t newheight;
	uint64_t bcount;

	//printf("FilePnode::InsertRegion region=%p\n", region);
	//printf("FilePnode::InsertRegion region->radixtree_.rnode_=%p\n", region->radixtree_.rnode_);
	//printf("FilePnode::InsertRegion region->base_bn_=%d\n", region->base_bn_);
	//printf("FilePnode::InsertRegion region->slot_.slot_base_=%p\n", region->slot_.slot_base_);

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
			// no slot, therefore region extends the bcobj
			// TODO: journal this update
			printf("extend the bcobj\n");
			printf("radixtree_.rnode_=%p\n", radixtree_.rnode_);
			if (radixtree_.rnode_) {
				// bcobj's radixtree exists so we need to move it under the
				// new region's radixtree. To do so, we first need to extend
				// the bcobj's radixtree to reach the height of the new
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
					radixtree_.Extend(session, bcount-1);
				}
				RadixTreeNode<Session>* node = region->radixtree_.rnode_;
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
template<typename Session>
int 
ByteContainer::Object<Session>::PatchSlot(ByteContainer::Slot<Session>& link)
{
	return 0;
}


// FIXME:
// The region object is destroyed when this function returns. What happens 
// to the persistent storage it points to??? For example, what would happen
// if someone else destroys the file??? We should still be able to 
// reference the blocks
// Perhaps we should use some refcounting
template<typename Session>
int 
ByteContainer::Object<Session>::ReadBlock(Session* session, 
                                          char* dst, 
                                          uint64_t bn, 
                                          int off, 
                                          int n)
{
	int l;
	int rn;

	printf("FilePnode::ReadBlock(dst=%p, bn=%llu, off=%d, n=%d)\n", dst, bn, off, n);

	l = min(dpo::common::BLOCK_SIZE, size_ - bn*dpo::common::BLOCK_SIZE);
	printf("size_=%d, l=%d\n", size_, l);
	if (l < off) {
		return 0;
	}
	rn = min(n, l-off);

	Region<Session> region(session, this, bn);
	region.ReadBlock(session, dst, bn, off, rn);
	for (int i=0; i<n; i++) {
		printf("dst[%d]=%x\n", i, dst[i]);
	}
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
template<typename Session>
int 
ByteContainer::Object<Session>::WriteBlock(Session* session, 
                                           char* src, 
                                           uint64_t bn, 
                                           int off, 
                                           int n)
{
	uint64_t                rbn;
	void**                  slot;
	RadixTreeNode<Session>* node;
	int                     offset;
	int            height;
	int            ret;
	char           buf[4096];

	printf("FilePnode::WriteBlock(src=%p, bn=%llu, off=%d, n=%d)\n", src, bn, off, n);

	Region<Session> region(session, this, bn);
	if ( (ret = region.WriteBlock(session, src, bn, off, n)) < 0) {
		return ret;
	}
	
	InsertRegion(session, &region);

	printf("FilePnode::WriteBlock: DONE\n");
	if ( (bn*dpo::common::BLOCK_SIZE + off + n) > size_ ) {
		size_ = bn*dpo::common::BLOCK_SIZE + off + n;
	}
	return n;
}


template<typename Session>
int 
ByteContainer::Object<Session>::Write(Session* session, char* src, uint64_t off, uint64_t n)
{
	return __Write<Session, ByteContainer::Object<Session> > (session, this, src, off, n);
}


template<typename Session>
int 
ByteContainer::Object<Session>::Read(Session* session, char* dst, uint64_t off, uint64_t n)
{
	return __Read<Session, ByteContainer::Object<Session> > (session, this, dst, off, n);
}


template<typename Session>
int 
ByteContainer::Object<Session>::LookupSlot(Session* session, 
                                           uint64_t bn, 
                                           Slot<Session>* slot)
{
	uint64_t                rbn;
	RadixTreeNode<Session>* node;
	int                     slot_offset;
	int                     slot_height;
	int                     ret;
	uint64_t                bcount;
	int                     i;

	if (!slot) {
		return -E_INVAL;
	}
	return slot->Init(session, this, bn);
}



/////////////////////////////////////////////////////////////////////////////
// 
// ByteContainer::Region
//
/////////////////////////////////////////////////////////////////////////////



// trusts the callee that it has already check that bn is out of range
template<typename Session>
int 
ByteContainer::Region<Session>::Extend(Session* session, uint64_t bn)
{
	// can only extend a region if the region is not allocated to an 
	// existing slot
	if (slot_.slot_base_) {
		return -1;
	}
	radixtree_.Extend(session, bn - base_bn_);
	maxbcount_ = 1 << ((radixtree_.height_)*RADIX_TREE_MAP_SHIFT);
	return 0;
}


// off is offset with respect to the block
template<typename Session>
int 
ByteContainer::Region<Session>::WriteBlock(Session* session, 
                                           char* src, 
                                           uint64_t bn, 
                                           int off, 
                                           int n)
{
	void**                  slot;
	char*                   bp;
	RadixTreeNode<Session>* node;
	int                     offset;
	int                     height;
	int                     ret;
	uint64_t                rbn;

	assert(off < dpo::common::BLOCK_SIZE);
	assert(off+n <= dpo::common::BLOCK_SIZE);

	printf("FilePnode::Region::WriteBlock(src=%p, bn=%llu, off=%d, n=%d)\n", src, bn, off, n);

	// check if block number out of range
	if (base_bn_ > bn) {
		return -E_INVAL;
	} else if (base_bn_+maxbcount_ <= bn) {
		if (Extend(session, bn) < 0) {
			// block out of range and cannot extend the region
			return -E_INVAL;
		}
	}

	if (maxbcount_ == 1) {
		slot = &dblock_;
	} else {
		rbn = bn - base_bn_;
		if ((ret = radixtree_.MapSlot(session, rbn, 1, 1, &node, &offset, &height)) == 0) {
			slot = &node->slots[offset];
		} else {
			return ret;
		}
	}	
	if (*slot == (void*)0) {
		void* ptr;
		if ((ret = session->salloc()->AllocateRaw(session, 
		                                        dpo::common::BLOCK_SIZE, &ptr)) < 0)
		{ 
			return ret;
		}
		*slot = ptr; // FIXME: journal this 
		bp = (char*) (*slot);
		printf("FilePnode::Region::WriteBlock block=%p\n", *slot);
		// TODO: Allocating and zeroing a chunk is done in other places in the 
		// code as well. We should collapse this under a function that does the job
		// (including any necessary journaling?)
		// Zero the part of the newly allocated block that is not written to
		// ensure we later read zeros and not garbage.
		if (off>0) {
			memset(bp, 0, off);
		}	
		memset(&bp[off+n], 0, dpo::common::BLOCK_SIZE-n); 
	}
	bp = (char*) (*slot);
	memmove(&bp[off], src, n);

	return n;
}


template<typename Session>
int 
ByteContainer::Region<Session>::Write(Session* session, 
                                      char* src, 
                                      uint64_t off, 
                                      uint64_t n)
{
	printf("FilePnode::Region::Write(src=%p, off=%lu, n=%lu)\n", src, off, n);

	return __Write<Session, ByteContainer::Region<Session> > (session, this, src, off, n);
}


template<typename Session>
int 
ByteContainer::Region<Session>::ReadBlock(Session* session, 
                                          char* dst, 
                                          uint64_t bn, 
                                          int off, 
                                          int n)
{
	void**                  slot;
	char*                   bp;
	RadixTreeNode<Session>* node;
	int                     offset;
	int                     height;
	int                     ret;
	uint64_t                rbn;

	assert(off < dpo::common::BLOCK_SIZE);
	assert(off+n <= dpo::common::BLOCK_SIZE);

	printf("FilePnode::Region::ReadBlock(dst=%p, bn=%llu, off=%d, n=%d)\n", dst, bn, off, n);

	if (base_bn_ > bn || base_bn_+maxbcount_ <= bn) {
		// block number out of range
		return -E_INVAL;
	}

	printf("FilePnode::Region::ReadBlock maxbcount_=%d\n", maxbcount_);

	if (maxbcount_ == 1) {
		slot = &dblock_;
	} else {
		rbn = bn - base_bn_;
		if (radixtree_.MapSlot(session, rbn, 1, 0, &node, &offset, &height) == 0) {
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


template<typename Session>
int 
ByteContainer::Region<Session>::Read(Session* session, char* dst, uint64_t off, uint64_t n)
{
	return __Read<Session, Region<Session> > (session, this, dst, off, n);
}


} // namespace common
} // namespace containers
} // namespace dpo

#endif // __STAMNOS_DPO_COMMON_BYTE_CONTAINER_H
