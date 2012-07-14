//! \file
//! Definition of the name collection persistent object stored in SCM
//!

//TODO: iterator interface should follow STL
//TODO: iterator interface should provide a more convenient way for initialization

#ifndef __STAMNOS_OSD_COMMON_BYTE_CONTAINER_OBJECT_H
#define __STAMNOS_OSD_COMMON_BYTE_CONTAINER_OBJECT_H

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <typeinfo>
#include "scm/scm/scm.h"
#include "osd/containers/byte/radixtree.h"
#include "osd/containers/containers.h"
#include "osd/main/common/obj.h"
#include "osd/main/common/const.h"
#include "osd/main/client/journal.h"
#include "osd/main/server/journal.h"
#include "scm/const.h"
#include "bcs/main/common/cdebug.h"
#include "common/util.h"
#include "common/prof.h"

//#define PROFILER_SAMPLE __PROFILER_SAMPLE


namespace osd {
namespace containers {
namespace common {

class ByteContainer {
public:
enum {
	N_DIRECT = 0
};

template<typename Session>
class Slot;

template<typename Session>
class Iterator;

template<typename Session>
class Region;

template<typename Session>
class Object: public osd::cc::common::Object {
public:
	static Object* Make(Session* session, osd::common::AclIdentifier acl_id = 0) {
		osd::common::ObjectId oid;
		
		if (session->salloc()->AllocateContainer(session, acl_id, T_BYTE_CONTAINER, &oid) < 0) {
			dbg_log(DBG_ERROR, "No storage available\n");
		}
		return Load(oid);
	}

	static Object* Make(Session* session, volatile char* ptr) {
		return new((void*) ptr) Object();
	}

	static Object* Load(osd::common::ObjectId oid) {
		return reinterpret_cast<Object*>(oid.addr());
	}
	
	static void Free(Session* session, osd::common::ObjectId oid);

	Object();

	int InsertRegion(Session* session, Region<Session>* region);
	int AllocateRegion(Session* session, uint64_t start_bn, uint64_t end_bn);

	int Write(Session* session, char* src, uint64_t off, uint64_t n);
	int Read(Session* session, char* dst, uint64_t off, uint64_t n);
	int ReadImmutable(Session* session, char* dst, uint64_t off, uint64_t n);

	int ReadBlock(Session* session, char* dst, uint64_t bn, int off, int n);
	int WriteBlock(Session* session, char* src, uint64_t bn, int off, int n);
	int LinkBlock(Session* session, uint64_t bn, void* bp);

	int LowerBound(Session* session, uint64_t bn, uint64_t* lge_bn);
	void PrintBlocks(Session* session);

	// return maximum possible size in bytes
	inline uint64_t get_maxsize() { 
		return get_maxbcount() * kBlockSize; 
	}

	// return maximum possible size in bytes
	inline uint64_t get_maxbcount() {
		uint64_t nblocks;

		nblocks = radixtree_.height_ == 0 ? 0 : (1 << ((radixtree_.height_)*RADIX_TREE_MAP_SHIFT));
		nblocks += N_DIRECT;

		return nblocks;
	}

	uint64_t Size() { return size_; }

	int LookupSlot(Session* session, uint64_t bn, Slot<Session>* slot);


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
			radixtree_.rnode_ = NULL;
			radixtree_.Extend(session, bn - N_DIRECT);
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
		if (!slot_.slot_base_) {
			return -1;
		}

		base_bn_ = slot_.base_bn_;
		if (base_bn_ < N_DIRECT) {
			maxbcount_ = 1;
			dblock_ = slot_.slot_base_[slot_.slot_offset_];
		} else {
			maxbcount_ = 1 << ((slot_.slot_height_-1)*RADIX_TREE_MAP_SHIFT);
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
	int LinkBlock(Session* session, uint64_t bn, void* bp);

	int Write(Session* session, char*, uint64_t, uint64_t);
	int Read(Session* session, char*, uint64_t, uint64_t);

	int Extend(Session* session, uint64_t bn);
	void** MapSlot(Session* session, uint64_t bn, bool alloc_slot);

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
	Iterator(Session* session = NULL)
		: session_(session)
	{ }

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
		return E_SUCCESS;
	}

	inline int NextSlot(Session* session) 
	{
		int        ret = 0;
		uint64_t   bcount;
		
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
block_valid_data(uint64_t bn, uint64_t file_size)
{
	return ((((bn + 1) * kBlockSize) < file_size ) 
	        ? kBlockSize 
	        : (file_size - (bn * kBlockSize)));
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
		bn = off / kBlockSize;
		f = off % kBlockSize;
		m = min(n - tot, kBlockSize - f);
		ret = obj->WriteBlock(session, &src[tot], bn, f, m);
		if (ret < 0 || ret < m) { // ret could be negative but comparing against unsigned
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
		bn = off / kBlockSize;
		f = off % kBlockSize;
		m = min(n - tot, kBlockSize - f);
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
void 
ByteContainer::Object<Session>::Free(Session* session, osd::common::ObjectId oid)
{
	Iterator<Session> start;
	Iterator<Session> iter;
	Object*           obj = Load(oid);
	int               n;
	
	start.Init(session, obj, 0);
	
	for (iter = start, n = 0; !iter.terminate() && n < obj->size_; iter++, n+=kBlockSize) 
	{
		uint64_t extent_u64 = (uint64_t) (*iter).slot_base_[(*iter).slot_offset_];
		if (extent_u64 != 0) {
			session->salloc()->FreeExtent(session, osd::common::ExtentId(extent_u64));
		}
	}
	session->salloc()->FreeContainer(session, oid);
}


template<typename Session>
int 
ByteContainer::Object<Session>::InsertRegion(Session* session, 
                                             ByteContainer::Region<Session>* region)
{
	uint64_t newheight;
	uint64_t bcount;

	if (region->base_bn_ < N_DIRECT) {
		assert(region->maxbcount_ == 1);
		// what if the region is already placed? 
		// need to compare first
		daddrs_[region->base_bn_] = region->dblock_;
	} else {
		if (region->slot_.slot_base_) {
			if (region->maxbcount_ > 1) {
				region->slot_.slot_base_[region->slot_.slot_offset_] = region->radixtree_.rnode_; 
			} else {
				region->slot_.slot_base_[region->slot_.slot_offset_] = region->dblock_; 
			}
		} else {
			// no slot, therefore region extends the bcobj
			if (radixtree_.rnode_) {
				// bcobj's radixtree exists so we need to move it under the
				// new region's radixtree. To do so, we first need to extend
				// the bcobj's radixtree to reach the height of the new
				// radixtree minus one. Then we can attach the old radixtree
				// to the new one.
				assert(radixtree_.height_ > 0 && 
				       region->radixtree_.height_ > radixtree_.height_);
				newheight = region->radixtree_.height_ - 1;
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
			}	
		}
	}

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

	//printf("FilePnode::ReadBlock(dst=%p, bn=%llu, off=%d, n=%d)\n", dst, bn, off, n);

	l = min(kBlockSize, size_ - bn*kBlockSize);
	if (l < off) {
		return 0;
	}
	rn = min(n, l-off);

	Region<Session> region(session, this, bn);
	region.ReadBlock(session, dst, bn, off, rn);
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
	int  ret;

	//printf("FilePnode::WriteBlock(src=%p, bn=%llu, off=%d, n=%d)\n", src, bn, off, n);

	Region<Session> region(session, this, bn);
	if ( (ret = region.WriteBlock(session, src, bn, off, n)) < 0) {
		return ret;
	}
	
	InsertRegion(session, &region);

	if ( (bn*kBlockSize + off + n) > size_ ) {
		size_ = bn*kBlockSize + off + n;
	}
	return n;
}


template<typename Session>
int 
ByteContainer::Object<Session>::LinkBlock(Session* session, uint64_t bn, void* bp)
{
	int  ret;

	Region<Session> region(session, this, bn);
	if ( (ret = region.LinkBlock(session, bn, bp)) < 0) {
		return ret;
	}
	
	InsertRegion(session, &region);

	if ( ((bn+1)*kBlockSize) > size_ ) {
		size_ = (bn+1)*kBlockSize;
	}
	return kBlockSize;
}


template<typename Session>
int 
ByteContainer::Object<Session>::Write(Session* session, char* src, uint64_t off, uint64_t n)
{
	return __Write<Session, ByteContainer::Object<Session> > (session, this, src, off, n);
}


template<typename Session>
int 
ByteContainer::Object<Session>::ReadImmutable(Session* session, 
                             char* dst, 
                             uint64_t off, 
                             uint64_t n)
{
	uint64_t                tot;
	uint64_t                m;
	uint64_t                fbn; // first block number
	uint64_t                bn;
	uint64_t                base_bn;
	ByteContainer::Iterator<Session> start;
	ByteContainer::Iterator<Session> iter;
	int                     ret;
	int                     f;
	uint64_t                bcount;
	uint64_t                size;
	char*                   ptr;

	//printf ("ReadImmutable: range = [%" PRIu64 " , %" PRIu64 " ]\n", off, off+n-1);

	// find out how much is really there to read
	if ((off + n) > Size()) {
		if (off > Size() ) {
			return 0;
		} else {
			n = min(Size() - off, n);
		}
	} 


	//dbg_log (DBG_DEBUG, "Immutable range = [%" PRIu64 ", %" PRIu64 "] n=%" PRIu64 "\n", off, off+n-1, n);
	//printf ("Immutable range = [%" PRIu64 ", %" PRIu64 "] n=%" PRIu64 "\n", off, off+n-1, n);

	fbn = off/kBlockSize;
	start.Init(session, this, fbn);
	iter = start;
	bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
	size = bcount * kBlockSize;
	f = off % size;


	for (tot=0, bn=fbn; 
	     !iter.terminate() && tot < n; 
	     iter++, tot+=m, off+=m, bn++) 
	{
		base_bn = (*iter).get_base_bn();
		bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
		size = bcount * kBlockSize;
		m = min(n - tot, size - f);

		ptr = (char*) (*iter).slot_base_[(*iter).slot_offset_];

		//printf("bn=%" PRIu64 " , base_bn = %" PRIu64 " , block=%p R[%d, %" PRIu64 "] A[%" PRIu64 " , %" PRIu64 " ] size=%" PRIu64 "  (%" PRIu64 "  blocks)\n", 
		//       bn, base_bn, ptr, f, f+m-1, off, off+m-1, size, bcount);

		if (!ptr) {
			memset(&dst[tot], 0, m);
		} else {
			// pinode already points to a block, therefore we do an in-place read
			assert(bcount == 1);
			memmove(&dst[tot], &ptr[f], m);
		}

		f = 0; // after the first block is read, each other block is read 
		       // starting at its first byte
	}

	return tot;
}



template<typename Session>
int 
ByteContainer::Object<Session>::Read(Session* session, char* dst, uint64_t off, uint64_t n)
{
	//return __Read<Session, ByteContainer::Object<Session> > (session, this, dst, off, n);
	return ReadImmutable (session, dst, off, n);
}


template<typename Session>
void
ByteContainer::Object<Session>::PrintBlocks(Session* session)
{
	int                     ret;
	void**                  slot;
	RadixTreeNode<Session>* node;
	int                     offset;
	int                     height;
	uint64_t                rbn;
	
	for (int bn=0; bn<16; bn++) {
		if ((ret = radixtree_.MapSlot(session, bn, 1, false, &node, &offset, &height)) == 0) {
			slot = &node->slots[offset];
		} else {
			slot = NULL;
		}
		if (slot) {
			printf("bn=%d, slot=%p, *slot=%p\n", bn, slot, *slot);
		} else {
			printf("bn=%d\n", bn);
		}
	}	
}


template<typename Session>
int 
ByteContainer::Object<Session>::LookupSlot(Session* session, 
                                           uint64_t bn, 
                                           Slot<Session>* slot)
{
	if (!slot) {
		return -E_INVAL;
	}
	return slot->Init(session, this, bn);
}


/**
 * Find the leftmost mapped block whose block number is greater than or 
 * equal to bn
 */
template<typename Session>
int 
ByteContainer::Object<Session>::LowerBound(Session* session, uint64_t bn, uint64_t* lge_bn)
{
	return radixtree_.LowerBound(session, bn, lge_bn);
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


/**
 * Returns a pointer to the slot corresponding to the block bn
 */
template<typename Session>
void**
ByteContainer::Region<Session>::MapSlot(Session* session, uint64_t bn, 
                                        bool alloc_slot)
{	
	int                     ret;
	void**                  slot;
	RadixTreeNode<Session>* node;
	int                     offset;
	int                     height;
	uint64_t                rbn;
	
	// check if block number out of range
	if (base_bn_ > bn) {
		return NULL;
	} else if (base_bn_+maxbcount_ <= bn) {
		if (alloc_slot) {
			if (Extend(session, bn) < 0) {
				// block out of range and cannot extend the region
				return NULL;
			}
		} else {
			return NULL;
		}
	}
	
	// map slot
	if (maxbcount_ == 1) {
		slot = &dblock_;
	} else {
		rbn = bn - base_bn_;
		if ((ret = radixtree_.MapSlot(session, rbn, 1, alloc_slot, &node, &offset, &height)) == 0) {
			slot = &node->slots[offset];
		} else {
			slot = NULL;
		}
	}	
	return slot;
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
	int                     ret;

	assert(off < kBlockSize);
	assert(off+n <= kBlockSize);

	//printf("FilePnode::Region::WriteBlock(src=%p, bn=%llu, off=%d, n=%d)\n", src, bn, off, n);

	if ((slot = MapSlot(session, bn, true)) == NULL) {
		return -E_INVAL;
	}
	if (*slot == (void*)0) {
		void* ptr;
		if ((ret = session->salloc()->AllocateExtent(session, kBlockSize, 
		                                             kData, &ptr)) < 0) { 
			return ret;
		}
		if (session->journal()->mode() == osd::common::Journal::Client) {
			session->journal() << osd::Publisher::Message::ContainerOperation::LinkBlock(slot_.bcobj_->oid(), bn, ptr);
		}
		*slot = ptr; // FIXME: journal this at server side
		
		bp = (char*) (*slot);
		// Zero the part of the newly allocated block that is not written to
		// ensure we later read zeros and not garbage.
		if (off>0) {
			memset(bp, 0, off);
		}	
		memset(&bp[off+n], 0, kBlockSize-n); 
	}
	bp = (char*) (*slot);
	//memmove(&bp[off], src, n); 
	scm_memcpy(&bp[off], src, n);

	return n;
}


// off is offset with respect to the block
template<typename Session>
int 
ByteContainer::Region<Session>::LinkBlock(Session* session, uint64_t bn, void* bp)
{
	void** slot;

	if ((slot = MapSlot(session, bn, true)) == NULL) {
		return -E_INVAL;
	}
	*slot = bp;
	return kBlockSize;
}



template<typename Session>
int 
ByteContainer::Region<Session>::Write(Session* session, 
                                      char* src, 
                                      uint64_t off, 
                                      uint64_t n)
{
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
	void**  slot;
	char*   bp;

	assert(off < kBlockSize);
	assert(off+n <= kBlockSize);

	if ((slot = MapSlot(session, bn, false)) == NULL) {
		return -E_INVAL;
	}
	if (slot && *slot) {
		bp = (char*) (*slot);
		//memmove(dst, &bp[off], n);
		memcpy(dst, &bp[off], n);
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
} // namespace osd

#endif // __STAMNOS_OSD_COMMON_BYTE_CONTAINER_H
