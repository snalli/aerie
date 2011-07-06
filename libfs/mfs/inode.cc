#include "mfs/inode.h"
#include <pthread.h>
#include <stdint.h>
#include "mfs/pinode.h"
#include "common/interval_tree.h"

class InodeInterval: public Interval {
public:
	InodeInterval(const int low, const int high)
		:low_(low), high_(high)
	{ }
	  
	inline int GetLowPoint() const { return low_;}
	inline int GetHighPoint() const { return high_;}
protected:
	int low_;
	int high_;
};


Inode::Inode()
{

	mpinode_ = new PInode;
	immpinode_ = NULL;
	intervaltree_ = NULL;
	preconstructed_intervaltree_ = NULL;
}


Inode::Inode(PInode* pinode)
{
	preconstructed_intervaltree_ = new IntervalTree();
	intervaltree_ = NULL;

	mpinode_ = NULL;
	immpinode_ = pinode;
}


Inode::~Inode()
{
	delete preconstructed_intervaltree_;
}


int Inode::ReadImmutable(char* dst, uint64_t off, uint64_t n)
{
	
}


int Inode::ReadMutable(char* dst, uint64_t off, uint64_t n)
{
	if (!mpinode_) {
		return -1;
	}


}


int Inode::Read(char* dst, uint64_t off, uint64_t n)
{
	uint64_t         immpinode_maxsize;
	uint64_t         mn;
	int              ret1;
	int              ret2;

	immpinode_maxsize = (immpinode_) ? immpinode_->get_maxsize() : 0;

	if (off + n < immpinode_maxsize) 
	{
		return ReadImmutable(dst, off, n);
	} else if ( off > immpinode_maxsize - 1) {
		return ReadMutable(dst, off, n);
	} else {
		mn = off + n - immpinode_maxsize; 
		ret1 = ReadImmutable(dst, off, n - mn);
		if (ret1 < 0) {
			return ret1;
		}
		ret2 = ReadMutable(&dst[n-mn], immpinode_maxsize, mn);
		if (ret2 < 0) {
			return ret1;
		}
		return ret1 + ret2;
	}
	assert(0); // unreachable
}


int Inode::WriteMutable(char* src, uint64_t off, uint64_t n)
{
	if (!mpinode_) {
		// FIXME need to write to the log a swap operation to the a 
		// new pinode if immutable pinode exists
		// FIXME actually this might be a swap for the radixtree...
		// FIXME PInode should support an operation where we swap radixtrees
		mpinode_ = new PInode;
	}

	return	mpinode_->Write(src, off, n);
}


int Inode::WriteImmutable(char* src, uint64_t off, uint64_t n)
{
	uint64_t         tot;
	uint64_t         m;
	uint64_t         fbn; // first block number
	uint64_t         lbn; // last block number
	uint64_t         bn;
	PInode::Iterator start;
	PInode::Iterator iter;
	InodeInterval*   result_interval;
	uint64_t         fb;
	uint64_t         lb;
	uint64_t         mn;
	int              ret1;
	int              ret2;

	fb = off/BLOCK_SIZE;
	lb = (off+n)/BLOCK_SIZE;
	
	bn = fbn;
	while (bn <= lbn) {
		//start.Init(pinode_, bn);
		result_interval = reinterpret_cast<InodeInterval*>(intervaltree_->LeftmostOverlap(bn, lbn));
		if (result_interval) {
			if ( result_interval->GetLowPoint() > bn ) {
				// create new interval
			} else {
				// write existing extents 
				//for 
			}
		}
	}


/*
	for(tot=0; tot<n; tot+=m, off+=m, src+=m) {
		pinode_->LookupBlock(off/BLOCK_SIZE);
		m = min(n - tot, BLOCK_SIZE - off%BLOCK_SIZE);
		memmove(off%BSIZE, src, m);
	}
*/

}



int Inode::Write(char* src, uint64_t off, uint64_t n)
{
	uint64_t         immpinode_maxsize;
	
	uint64_t         mn;
	int              ret1;
	int              ret2;

	immpinode_maxsize = (immpinode_) ? immpinode_->get_maxsize() : 0;

	if (off + n < immpinode_maxsize) 
	{
		return WriteImmutable(src, off, n);
	} else if ( off >= immpinode_maxsize) {
		return WriteMutable(src, off, n);
	} else {
		mn = off + n - immpinode_maxsize; 
		ret1 = WriteImmutable(src, off, n - mn);
		if (ret1 < 0) {
			return ret1;
		}
		ret2 = WriteMutable(&src[n-mn], immpinode_maxsize, mn);
		if (ret2 < 0) {
			return ret1;
		}
		return ret1 + ret2;
	}
	assert(0); // unreachable
}

