#include "dpo/containers/byte/container.h"
#include <stdint.h>
#include <inttypes.h>
#include <vector>
#include "common/errno.h"
#include "client/session.h"
#include "dpo/main/client/salloc.h"
#include "common/interval_tree.h"

namespace dpo {
namespace containers {
namespace client {

#define min(a,b) ((a) < (b)? (a) : (b))
#define max(a,b) ((a) > (b)? (a) : (b))

/////////////////////////////////////////////////////////////////////////////
// 
// Helper Class: ByteInterval
//
/////////////////////////////////////////////////////////////////////////////


const int INTERVAL_MIN_SIZE = 64;

class ByteInterval: public Interval {
public:
	ByteInterval(const int low, const int high)
		: low_(low), 
		  high_(high)
	{ }

	ByteInterval(::client::Session* session, ByteContainer::Slot& slot, const int low, const int size)
		: low_(low), 
		  high_(low+size-1), 
		  slot_(slot)
	{ 
		assert(slot.slot_base_);
		if (size <= INTERVAL_MIN_SIZE) {
			// Adjust the slot offset to be multiple of interval size
			slot_.slot_offset_ &= ~(INTERVAL_MIN_SIZE-1);
			block_array_ = new char *[INTERVAL_MIN_SIZE];
			for (int i=0; i<INTERVAL_MIN_SIZE; i++) {
				block_array_[i] = 0;
			}
			region_ = NULL;
		} else {
			block_array_ = NULL;
			region_ = new ByteContainer::Region(session, slot); // FIXME: pass session
		}	

		printf("interval: slot_base=%p slot_offset=%d, range=[%" PRIu64 ", %" PRIu64 "]\n", 
		       slot_.slot_base_, slot_.slot_offset_, low_, high_);
	}
	  
	inline int GetLowPoint() const { return low_;}
	inline int GetHighPoint() const { return high_;}
	int Write(::client::Session* session, char*, uint64_t, uint64_t);
	int Read(::client::Session* session, char*, uint64_t, uint64_t);

protected:
	uint64_t                       low_;
	uint64_t                       high_;
	ByteContainer::Region*         region_;     
	ByteContainer::Slot            slot_;       // needed when region_ is NULL
	char**                         block_array_;

	int WriteBlockNoRegion(::client::Session* session, char*, uint64_t, int, int);
	int WriteNoRegion(::client::Session* session, char*, uint64_t, uint64_t);
	int ReadBlockNoRegion(::client::Session* session, char*, uint64_t, int, int);
	int ReadNoRegion(::client::Session* session, char*, uint64_t, uint64_t);
};


// TODO: need to journal each new allocated block and link
int
ByteInterval::WriteBlockNoRegion(::client::Session* session, char* src, uint64_t bn, int off, int n)
{
	char* bp;

	assert(low_ <= bn && bn <= high_);

	printf("ByteInterval::WriteBlock (src=%p, bn=%" PRIu64 ", off=%d, n=%d)\n",
	       src, bn, off, n);
	
	if (!(bp = block_array_[bn - low_])) {
		printf("ByteInterval::WriteBlock Allocate Block\n",
	       src, bn, off, n);
		// allocate new block, FIXME: need to journal the alloaction and link
		block_array_[bn - low_] = (char*) malloc(dpo::common::BLOCK_SIZE); // FIXME: allocate a chunk
		bp = block_array_[bn - low_];
		
		// TODO: Allocating and zeroing a chunk is done in other places in the 
		// code as well. We should collapse this under a function that does the job
		// (including any necessary journaling?)
		// Zero the part of the newly allocated block that is not written to
		// ensure we later read zeros and not garbage.
		if (off>0) {
			memset(bp, 0, off);
		}	
		memset(&bp[off+n], 0, dpo::common::BLOCK_SIZE-n); 
	
		// TODO: create and journal physical and logical links
		// physical links can be created using information kept in slot_:
		//  - we know the slot's base
		//  - we know the slot's offset which is (bn - low_)+slot_.slot_offset_
	}

	memmove(&bp[off], src, n);
	
}


int
ByteInterval::WriteNoRegion(::client::Session* session, char* src, uint64_t off, uint64_t n)
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
		ret = WriteBlockNoRegion(session, &src[tot], bn, f, m);
		if (ret < 0) {
			return ((ret < 0) ? ( (tot>0)? tot: ret)  
			                  : tot + ret);
		}
	}

	return tot;
}


int
ByteInterval::Write(::client::Session* session, char* src, uint64_t off, uint64_t n)
{
	printf("Buffered Write [%" PRIu64 ", %" PRIu64 "], region=%p\n", off, off+n-1, region_);

	if (region_) {
		return region_->Write(session, src, off, n);
	} else {
		return WriteNoRegion(session, src, off, n);
	}
}


int
ByteInterval::ReadBlockNoRegion(::client::Session* session, char* dst, uint64_t bn, int off, int n)
{
	char* bp;

	assert(low_ <= bn && bn <= high_);

	printf("ByteInterval::ReadBlock (dst=%p, bn=%" PRIu64 ", off=%d, n=%d)\n",
	       dst, bn, off, n);
	
	if (!(bp = block_array_[bn - low_])) {
		memset(dst, 0, n);
	}

	memmove(dst, &bp[off], n);
}


int
ByteInterval::ReadNoRegion(::client::Session* session, char* dst, uint64_t off, uint64_t n)
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
		ret = ReadBlockNoRegion(session, &dst[tot], bn, f, m);
		if (ret < 0) {
			return ((ret < 0) ? ( (tot>0)? tot: ret)  
			                  : tot + ret);
		}
	}

	return tot;
}


int
ByteInterval::Read(::client::Session* session, char* dst, uint64_t off, uint64_t n)
{
	printf("Buffered Read [%" PRIu64 ", %" PRIu64 "], region=%p\n", off, off+n-1, region_);

	if (region_) {
		return region_->Read(session, dst, off, n);
	} else {
		return ReadNoRegion(session, dst, off, n);
	}
}


/////////////////////////////////////////////////////////////////////////////
// 
// ByteContainer Verion Manager: Logical Copy-On-Write 
//
/////////////////////////////////////////////////////////////////////////////



int 
ByteContainer::VersionManager::vOpen()
{
	// FIXME: check if object is private or public. If private
	// then mark it as directly mutable
	
	dpo::vm::client::VersionManager<ByteContainer::Object>::vOpen();
	
	if (0 /* private */) {
		mutable_ = true;
		intervaltree_ = NULL;
	} else {
		mutable_ = false;
		intervaltree_ = new IntervalTree();
	}
	region_ = NULL;
	size_ = object()->Size();

	return E_SUCCESS;
}



// FIXME: Currently we publish by simply doing the updates in-place. 
// Normally this must be done via the trusted server using the journal 
int 
ByteContainer::VersionManager::vUpdate(::client::Session* session)
{
	int                   ret;

	dpo::vm::client::VersionManager<ByteContainer::Object>::vUpdate(session);

	// TODO

	return 0;
}


int 
ByteContainer::VersionManager::ReadImmutable(::client::Session* session, 
                                             char* dst, 
                                             uint64_t off, 
                                             uint64_t n)
{
	uint64_t                tot;
	uint64_t                m;
	uint64_t                fbn; // first block number
	uint64_t                bn;
	uint64_t                base_bn;
	ByteContainer::Iterator start;
	ByteContainer::Iterator iter;
	int                     ret;
	int                     f;
	uint64_t                bcount;
	uint64_t                size;
	char*                   ptr;
	uint64_t                interval_size;
	uint64_t                interval_low;
	ByteInterval*           interval;

	dbg_log (DBG_DEBUG, "Immutable range = [%" PRIu64 ", %" PRIu64 "] n=%" PRIu64 "\n", off, off+n-1, n);

	fbn = off/dpo::common::BLOCK_SIZE;
	start.Init(session, object(), fbn);
	iter = start;
	bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
	size = bcount * dpo::common::BLOCK_SIZE;
	f = off % size;


	for (tot=0, bn=fbn; 
	     !iter.terminate() && tot < n; 
	     iter++, tot+=m, off+=m, bn++) 
	{
		base_bn = (*iter).get_base_bn();
		bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
		size = bcount * dpo::common::BLOCK_SIZE;
		m = min(n - tot, size - f);

		ptr = (char*) (*iter).slot_base_[(*iter).slot_offset_];

		printf("bn=%" PRIu64 " , base_bn = %" PRIu64 " , block=%p R[%d, %" PRIu64 "] A[%" PRIu64 " , %" PRIu64 " ] size=%" PRIu64 "  (%" PRIu64 "  blocks)\n", 
		       bn, base_bn, ptr, f, f+m-1, off, off+m-1, size, bcount);

		if (!ptr) {
			// Downcasting via a static cast is generally dangerous, but we know 
			// that Interval is always of type ByteInterval so it should be safe

			//TODO: Optimization: we should check whether the block falls in the last 
			//interval to save a lookup.
			interval = static_cast<ByteInterval*>(intervaltree_->LeftmostOverlap(bn, bn));
			if (!interval) {
				// return zeros
				memset(&dst[tot], 0, m);
			} else {
				if (ret = interval->Read(session, &dst[tot], off, m) < m) {
					return ((ret < 0) ? ( (tot>0)? tot: ret)  
					                  : tot + ret);
				}
			}
		} else {
			// pinode already points to a block, therefore we do an in-place write
			assert(bcount == 1);

			printf("Direct Read [%" PRIu64 " , %" PRIu64 " ]\n", off, off+m-1);

			memmove(&dst[tot], &ptr[f], m);
		}

		f = 0; // after the first block is read, each other block is read 
		       // starting at its first byte
	}

	return tot;
}


int 
ByteContainer::VersionManager::ReadMutable(::client::Session* session, 
                                           char* dst, 
                                           uint64_t off, 
                                           uint64_t n)
{
	int vn;

	dbg_log (DBG_DEBUG, "Mutable range = [%" PRIu64 " , %" PRIu64 " ]\n", off, off+n-1);

	if (off > size_) {
		return 0;
	}
	vn = min(size_ - off, n);

	if (mutable_) {
		assert(region_ == NULL);
		return object()->Read(session, dst, off, vn);
	} else if (region_) {
		return	region_->Read(session, dst, off, vn);
	}

	return 0;
}


int 
ByteContainer::VersionManager::Read(::client::Session* session, 
                                    char* dst, 
                                    uint64_t off, 
                                    uint64_t n)
{
	uint64_t  immmaxsize; // immutable range max size
	uint64_t  mn;
	int       ret1 = 0;
	int       ret2 = 0;
	int       r;

	immmaxsize = (!mutable_) ? object()->get_maxsize(): 0;

	if (off + n < immmaxsize) 
	{
		ret1 = ReadImmutable(session, dst, off, n);
	} else if ( off > immmaxsize - 1) {
		ret2 = ReadMutable(session, dst, off, n);
	} else {
		mn = off + n - immmaxsize; 
		ret1 = ReadImmutable(session, dst, off, n - mn);
		// If ReadImmutable read less than what we asked for 
		// then we should short circuit and return because POSIX
		// semantics require us to return the number of contiguous
		// bytes read. Is this true?
		if (ret1 < n - mn) {
			return ret1;
		}
		ret2 = ReadMutable(session, &dst[n-mn], immmaxsize, mn);
		if (ret2 < 0) {
			ret2 = 0;
		}
	}

	if (ret1 < 0 || ret2 < 0) {
		return -1;
	}
	r = ret1 + ret2;

	return r;
}


int 
ByteContainer::VersionManager::WriteMutable(::client::Session* session, 
                                            char* src, 
                                            uint64_t off, 
                                            uint64_t n)
{
	uint64_t bn;

	dbg_log (DBG_DEBUG, "Mutable range = [%" PRIu64 " , %" PRIu64 " ]\n", off, off+n-1);

	// TODO: do we need to perform any journaling here? OR, does the FilePnode::Region
	// transparently perform any necessary journaling? 
	// For example, do we need to journal
	// 1) a swap operation to the new pinode if immutable pinode exists?
	// 2) a swap to the radixtree...?

	if (mutable_) {
		assert(region_ == NULL);
		return object()->Write(session, src, off, n);
	} else if (!region_) {
		bn = off/dpo::common::BLOCK_SIZE;
		region_ = new ByteContainer::Region(session, object(), bn);
	}
	return	region_->Write(session, src, off, n);
}


int 
ByteContainer::VersionManager::WriteImmutable(::client::Session* session, 
                                              char* src, 
                                              uint64_t off, 
                                              uint64_t n)
{
	uint64_t                tot;
	uint64_t                m;
	uint64_t                fbn; // first block number
	uint64_t                bn;
	uint64_t                base_bn;
	ByteContainer::Iterator start;
	ByteContainer::Iterator iter;
	int                     ret;
	int                     f;
	uint64_t                bcount;
	uint64_t                size;
	char*                   ptr;
	uint64_t                interval_size;
	uint64_t                interval_low;
	ByteInterval*           interval;

	dbg_log (DBG_DEBUG, "Immutable range = [%" PRIu64 " , %" PRIu64 " ] n=%" PRIu64 " \n", off, off+n-1, n);

	fbn = off/dpo::common::BLOCK_SIZE;
	start.Init(session, object(), fbn);
	iter = start;
	bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
	size = bcount * dpo::common::BLOCK_SIZE;
	f = off % size;


	for (tot=0, bn=fbn; 
	     !iter.terminate() && tot < n; 
	     iter++, tot+=m, off+=m, bn++) 
	{
		base_bn = (*iter).get_base_bn();
		bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
		size = bcount * dpo::common::BLOCK_SIZE;
		m = min(n - tot, size - f);

		ptr = (char*) ((*iter).slot_base_[(*iter).slot_offset_]);

		printf("bn=%" PRIu64 " , base_bn = %" PRIu64 " , block=%p R[%d, %" PRIu64 "] A[%" PRIu64 " , %" PRIu64 " ] size=%" PRIu64 "  (%" PRIu64 "  blocks)\n", 
		       bn, base_bn, ptr, f, f+m-1, off, off+m-1, size, bcount);

		if (!ptr) {
			// Downcasting via a static cast is generally dangerous, but we know 
			// that Interval is always of type ByteInterval so it should be safe

			//TODO: Optimization: we should check whether the block falls in the last 
			//interval to save a lookup.
			interval = static_cast<ByteInterval*>(intervaltree_->LeftmostOverlap(bn, bn));
			if (!interval) {
				// create new interval
				if (bn < ::dpo::containers::common::ByteContainer::N_DIRECT) {
					interval_size = 8;
					interval_low = 0;
				} else {
					interval_size = max(bcount, INTERVAL_MIN_SIZE);
					interval_low = ::dpo::containers::common::ByteContainer::N_DIRECT + 
					               ((base_bn-::dpo::containers::common::ByteContainer::N_DIRECT) & ~(interval_size - 1));
				}
				interval = new ByteInterval(session, (*iter), interval_low, interval_size);
				intervaltree_->Insert(interval);
			}

			if (ret = interval->Write(session, &src[tot], off, m) < m) {
				return ((ret < 0) ? ( (tot>0)? tot: ret)  
				                  : tot + ret);
			}
		} else {
			// pinode already points to a block, therefore we do an in-place write

			// TODO: if we want to support copy on write then we should not 
			// overwrite the block but instead go through the inverval and if needed 
			// copy the old contents of the block if a partial copy is done
			assert(bcount == 1);

			printf("Direct Write [%" PRIu64 " , %" PRIu64 " ]\n", off, off+m-1);
			memmove(&ptr[f], &src[tot], m);
		}

		f = 0; // after the first block is written, each other block is written 
		       // starting at its first byte
	}

	return tot;
}


int 
ByteContainer::VersionManager::Write(::client::Session* session, 
                                     char* src, 
                                     uint64_t off, 
                                     uint64_t n)
{
	uint64_t  immmaxsize; // immutable range max size
	uint64_t  mn;
	int       ret1 = 0;
	int       ret2 = 0;
	int       w;
	
	dbg_log (DBG_DEBUG, "Write range = [%" PRIu64 " , %" PRIu64 " ] n=%" PRIu64 " \n", off, off+n-1, n);

	immmaxsize = (!mutable_) ? object()->get_maxsize() : 0;

	if (off + n < immmaxsize) 
	{
		ret1 = WriteImmutable(session, src, off, n);
	} else if ( off >= immmaxsize) {
		ret2 = WriteMutable(session, src, off, n);
	} else {
		mn = off + n - immmaxsize; 
		ret1 = WriteImmutable(session, src, off, n - mn);
		// If WriteImmutable wrote less than what we asked for 
		// then we should short circuit and return because POSIX
		// semantics require us to return the number of contiguous
		// bytes written. Is this true?
		if (ret1 < n - mn) {
			return ret1;
		}
		ret2 = WriteMutable(session, &src[n-mn], immmaxsize, mn);
		if (ret2 < 0) {
			ret2 = 0;
		}
	}

	if (ret1 < 0 || ret2 < 0) {
		return -1;
	}
	w = ret1 + ret2;

	if ( (off + w) > size_ ) {
		size_ = off + w;
	}

	return w;
}


int
ByteContainer::VersionManager::Size(::client::Session* session)
{
	return size_;
}

} // namespace dpo
} // namespace containers
} // namespace client
