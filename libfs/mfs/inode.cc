#include "mfs/inode.h"
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <bitset>
#include <vector>
#include "mfs/pinode.h"
#include "common/interval_tree.h"

#define min(a,b) ((a) < (b)? (a) : (b))
#define max(a,b) ((a) > (b)? (a) : (b))


/////////////////////////////////////////////////////////////////////////////
// 
// InodeInterval
//
/////////////////////////////////////////////////////////////////////////////


const int INTERVAL_MIN_SIZE = 64;

class InodeInterval: public Interval {
public:
	InodeInterval(const int low, const int high)
		: low_(low), 
		  high_(high)
	{ }

	InodeInterval(PInode::Slot& slot, const int low, const int size)
		: low_(low), 
		  high_(low+size-1), 
		  slot_(slot)
	{ 
		assert(slot.slot_base_);
		if (size <= INTERVAL_MIN_SIZE) {
			// Adjust the slot offset to be multiple of interval size
			slot_.slot_offset_ &= ~(INTERVAL_MIN_SIZE-1);
			block_array_ = new char *[INTERVAL_MIN_SIZE];
			region_ = NULL;
		} else {
			block_array_ = NULL;
			region_ = new PInode::Region(slot);
		}	

		printf("interval: slot_base=%p slot_offset=%d, range=[%llu, %llu]\n", slot_.slot_base_, slot_.slot_offset_, low_, high_);
	}
	  
	inline int GetLowPoint() const { return low_;}
	inline int GetHighPoint() const { return high_;}
	int Write(char*, uint64_t, uint64_t);
	int Read(char*, uint64_t, uint64_t);

protected:
	std::bitset<INTERVAL_MIN_SIZE> bitmap_;
	uint64_t                       low_;
	uint64_t                       high_;
	PInode::Region*                region_;     
	PInode::Slot                   slot_;       // needed when region_ is NULL
	char**                         block_array_;

	int WriteBlockNoRegion(char*, uint64_t, int, int);
	int WriteNoRegion(char*, uint64_t, uint64_t);
	int ReadBlockNoRegion(char*, uint64_t, int, int);
	int ReadNoRegion(char*, uint64_t, uint64_t);
};


// TODO: need to journal each new allocated block and link
int
InodeInterval::WriteBlockNoRegion(char* src, uint64_t bn, int off, int n)
{
	char* bp;

	assert(low_ <= bn && bn <= high_);

	printf("InodeInterval::WriteBlock (src=%p, bn=%llu, off=%d, n=%d)\n",
	       src, bn, off, n);
	
	if (!(bp = block_array_[bn - low_])) {
		printf("InodeInterval::WriteBlock Allocate Block\n",
	       src, bn, off, n);
		// allocate new block, FIXME: need to journal the alloaction and link
		block_array_[bn - low_] = (char*) malloc(BLOCK_SIZE); // FIXME: allocate a chunk
		bp = block_array_[bn - low_];
		// TODO: Allocating and zeroing a chunk is done in other places in the 
		// code as well. We should collapse this under a function that does the job
		// (including any necessary journaling?)
		// Zero the part of the newly allocated block that is not written to
		// ensure we later read zeros and not garbage.
		if (off>0) {
			memset(bp, 0, off);
		}	
		memset(&bp[off+n], 0, BLOCK_SIZE-n); 
	
		// TODO: create and journal physical and logical links
		// physical links can be created using information kept in slot_:
		//  - we know the slot's base
		//  - we know the slot's offset which is (bn - low_)+slot_.slot_offset_
	}

	memmove(&bp[off], src, n);
	
}


int
InodeInterval::WriteNoRegion(char* src, uint64_t off, uint64_t n)
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
		ret = WriteBlockNoRegion(&src[tot], bn, f, m);
		if (ret < 0) {
			return ((ret < 0) ? ( (tot>0)? tot: ret)  
			                  : tot + ret);
		}
	}

	return tot;
}


int
InodeInterval::Write(char* src, uint64_t off, uint64_t n)
{
	printf("Buffered Write [%llu, %llu], region=%p\n", off, off+n-1, region_);

	if (region_) {
		return region_->Write(src, off, n);
	} else {
		return WriteNoRegion(src, off, n);
	}
}


int
InodeInterval::ReadBlockNoRegion(char* dst, uint64_t bn, int off, int n)
{
	char* bp;

	assert(low_ <= bn && bn <= high_);

	printf("InodeInterval::ReadBlock (dst=%p, bn=%llu, off=%d, n=%d)\n",
	       dst, bn, off, n);
	
	if (!(bp = block_array_[bn - low_])) {
		memset(dst, 0, n);
	}

	memmove(dst, &bp[off], n);
}


int
InodeInterval::ReadNoRegion(char* dst, uint64_t off, uint64_t n)
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
		ret = ReadBlockNoRegion(&dst[tot], bn, f, m);
		if (ret < 0) {
			return ((ret < 0) ? ( (tot>0)? tot: ret)  
			                  : tot + ret);
		}
	}

	return tot;
}


int
InodeInterval::Read(char* dst, uint64_t off, uint64_t n)
{
	printf("Buffered Read [%llu, %llu], region=%p\n", off, off+n-1, region_);

	if (region_) {
		return region_->Read(dst, off, n);
	} else {
		return ReadNoRegion(dst, off, n);
	}
}


/////////////////////////////////////////////////////////////////////////////
// 
// Inode
//
/////////////////////////////////////////////////////////////////////////////


Inode::Inode()
	: pinodeism_(true),
	  intervaltree_(NULL),
	  region_(NULL),
	  size_(0)
{
	pinode_ = new PInode;
}


Inode::Inode(PInode* pinode)
	: pinode_(pinode),
	  pinodeism_(false),
	  region_(NULL)
{
	intervaltree_ = new IntervalTree();
	size_ = pinode_->get_size();
}


Inode::~Inode()
{
	if (pinode_) {
		// TODO: what to do? inode is destroyed and the pinode has not been published yet?
		// need a flag to know whether we have published the pinode?
	}

	if (intervaltree_) {
		delete intervaltree_;
	}	
}


int Inode::ReadImmutable(char* dst, uint64_t off, uint64_t n)
{
	uint64_t         tot;
	uint64_t         m;
	uint64_t         fbn; // first block number
	uint64_t         bn;
	uint64_t         base_bn;
	PInode::Iterator start;
	PInode::Iterator iter;
	int              ret;
	int              f;
	uint64_t         bcount;
	uint64_t         size;
	char*            ptr;
	uint64_t         interval_size;
	uint64_t         interval_low;
	InodeInterval*   interval;

	dbg_log (DBG_DEBUG, "Immutable range = [%llu, %llu] n=%llu\n", off, off+n-1, n);

	fbn = off/BLOCK_SIZE;
	start.Init(pinode_, fbn);
	iter = start;
	bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
	size = bcount * BLOCK_SIZE;
	f = off % size;


	for (tot=0, bn=fbn; 
	     !iter.terminate() && tot < n; 
	     iter++, tot+=m, off+=m, bn++) 
	{
		base_bn = (*iter).get_base_bn();
		bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
		size = bcount * BLOCK_SIZE;
		m = min(n - tot, size - f);

		ptr = (char*) (*iter).slot_base_[(*iter).slot_offset_];

		printf("bn=%llu, base_bn = %llu, block=%p R[%d, %d] A[%llu, %llu] size=%llu (%llu blocks)\n", 
		       bn, base_bn, ptr, f, f+m-1, off, off+m-1, size, bcount);

		if (!ptr) {
			// Downcasting via a static cast is generally dangerous, but we know 
			// that Interval is always of type InodeInterval so it should be safe

			//TODO: Optimization: we should check whether the block falls in the last 
			//interval to save a lookup.
			interval = static_cast<InodeInterval*>(intervaltree_->LeftmostOverlap(bn, bn));
			if (!interval) {
				// return zeros
				memset(&dst[tot], 0, m);
			} else {
				if (ret = interval->Read(&dst[tot], off, m) < m) {
					return ((ret < 0) ? ( (tot>0)? tot: ret)  
					                  : tot + ret);
				}
			}
		} else {
			// pinode already points to a block, therefore we do an in-place write
			assert(bcount == 1);

			printf("Direct Read [%llu, %llu]\n", off, off+m-1);

			memmove(&dst[tot], &ptr[f], m);
		}

		f = 0; // after the first block is read, each other block is read 
		       // starting at its first byte
	}

	return tot;
}






int Inode::ReadMutable(char* dst, uint64_t off, uint64_t n)
{
	int vn;

	dbg_log (DBG_DEBUG, "Mutable range = [%llu, %llu]\n", off, off+n-1);

	if (off > size_) {
		return 0;
	}
	vn = min(size_ - off, n);

	if (pinodeism_) {
		assert(region_ == NULL);
		return pinode_->Read(dst, off, vn);
	} else if (region_) {
		return	region_->Read(dst, off, vn);
	}

	return 0;
}


int Inode::Read(char* dst, uint64_t off, uint64_t n)
{
	uint64_t  immmaxsize; // immutable range max size
	uint64_t  mn;
	int       ret1 = 0;
	int       ret2 = 0;
	int       r;

	immmaxsize = (!pinodeism_) ? pinode_->get_maxsize() : 0;

	if (off + n < immmaxsize) 
	{
		ret1 = ReadImmutable(dst, off, n);
	} else if ( off > immmaxsize - 1) {
		ret2 = ReadMutable(dst, off, n);
	} else {
		mn = off + n - immmaxsize; 
		ret1 = ReadImmutable(dst, off, n - mn);
		// If ReadImmutable read less than what we asked for 
		// then we should short circuit and return because POSIX
		// semantics require us to return the number of contiguous
		// bytes read. Is this true?
		if (ret1 < n - mn) {
			return ret1;
		}
		ret2 = ReadMutable(&dst[n-mn], immmaxsize, mn);
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


int Inode::WriteMutable(char* src, uint64_t off, uint64_t n)
{
	uint64_t bn;

	dbg_log (DBG_DEBUG, "Mutable range = [%llu, %llu]\n", off, off+n-1);

	// TODO: do we need to perform any journaling here? OR, does the PInode::Region
	// transparently perform any necessary journaling? 
	// For example, do we need to journal
	// 1) a swap operation to the new pinode if immutable pinode exists?
	// 2) a swap to the radixtree...?

	if (pinodeism_) {
		assert(region_ == NULL);
		return pinode_->Write(src, off, n);
	} else {
		if (!region_) {
			bn = off/BLOCK_SIZE;
			region_ = new PInode::Region(pinode_, bn);
		}
		return	region_->Write(src, off, n);
	}

	dbg_log (DBG_ERROR, "Unreachable code reached!\n");
}


int Inode::WriteImmutable(char* src, uint64_t off, uint64_t n)
{
	uint64_t         tot;
	uint64_t         m;
	uint64_t         fbn; // first block number
	uint64_t         bn;
	uint64_t         base_bn;
	PInode::Iterator start;
	PInode::Iterator iter;
	int              ret;
	int              f;
	uint64_t         bcount;
	uint64_t         size;
	char*            ptr;
	uint64_t         interval_size;
	uint64_t         interval_low;
	InodeInterval*   interval;

	dbg_log (DBG_DEBUG, "Immutable range = [%llu, %llu] n=%llu\n", off, off+n-1, n);

	fbn = off/BLOCK_SIZE;
	start.Init(pinode_, fbn);
	iter = start;
	bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
	size = bcount * BLOCK_SIZE;
	f = off % size;


	for (tot=0, bn=fbn; 
	     !iter.terminate() && tot < n; 
	     iter++, tot+=m, off+=m, bn++) 
	{
		base_bn = (*iter).get_base_bn();
		bcount = 1 << (((*iter).slot_height_ - 1)*RADIX_TREE_MAP_SHIFT);
		size = bcount * BLOCK_SIZE;
		m = min(n - tot, size - f);

		ptr = (char*) ((*iter).slot_base_[(*iter).slot_offset_]);

		printf("bn=%llu, base_bn = %llu, block=%p R[%d, %d] A[%llu, %llu] size=%llu (%llu blocks)\n", 
		       bn, base_bn, ptr, f, f+m-1, off, off+m-1, size, bcount);

		if (!ptr) {
			// Downcasting via a static cast is generally dangerous, but we know 
			// that Interval is always of type InodeInterval so it should be safe

			//TODO: Optimization: we should check whether the block falls in the last 
			//interval to save a lookup.
			interval = static_cast<InodeInterval*>(intervaltree_->LeftmostOverlap(bn, bn));
			if (!interval) {
				// create new interval
				if (bn < N_DIRECT) {
					interval_size = 8;
					interval_low = 0;
				} else {
					interval_size = max(bcount, INTERVAL_MIN_SIZE);
					interval_low = N_DIRECT + 
					               ((base_bn-N_DIRECT) & ~(interval_size - 1));
				}
				interval = new InodeInterval((*iter), interval_low, 
				                             interval_size);
				intervaltree_->Insert(interval);
			}

			if (ret = interval->Write(&src[tot], off, m) < m) {
				return ((ret < 0) ? ( (tot>0)? tot: ret)  
				                  : tot + ret);
			}
		} else {
			// pinode already points to a block, therefore we do an in-place write

			// TODO: if we want to support copy on write then we should not 
			// overwrite the block but instead go through the inverval and if needed 
			// copy the old contents of the block if a partial copy is done
			assert(bcount == 1);

			printf("Direct Write [%llu, %llu]\n", off, off+m-1);
			memmove(&ptr[f], &src[tot], m);
		}

		f = 0; // after the first block is written, each other block is written 
		       // starting at its first byte
	}

	return tot;
}


int Inode::Write(char* src, uint64_t off, uint64_t n)
{
	uint64_t  immmaxsize; // immutable range max size
	uint64_t  mn;
	int       ret1 = 0;
	int       ret2 = 0;
	int       w;

	dbg_log (DBG_DEBUG, "Write range = [%llu, %llu] n=%llu\n", off, off+n-1, n);

	immmaxsize = (!pinodeism_) ? pinode_->get_maxsize() : 0;

	if (off + n < immmaxsize) 
	{
		ret1 = WriteImmutable(src, off, n);
	} else if ( off >= immmaxsize) {
		ret2 = WriteMutable(src, off, n);
	} else {
		mn = off + n - immmaxsize; 
		ret1 = WriteImmutable(src, off, n - mn);
		// If WriteImmutable wrote less than what we asked for 
		// then we should short circuit and return because POSIX
		// semantics require us to return the number of contiguous
		// bytes written. Is this true?
		if (ret1 < n - mn) {
			return ret1;
		}
		ret2 = WriteMutable(&src[n-mn], immmaxsize, mn);
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


int Inode::Publish()
{
	dbg_log (DBG_CRITICAL, "Functionality not yet implemented!\n");

	// TODO publish inode to the world
	// Need to merge the mutable region with the immutable pinode
	// Need to communicate with the file system server
}
