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

}


Inode::Inode(PInode* pinode)
	
{
	preconstructed_intervaltree_ = new IntervalTree();
	intervaltree_ = NULL;


}

Inode::~Inode()
{
	delete preconstructed_intervaltree_;
}


int Inode::Read(char* dst, uint64_t off, uint64_t n)
{
	InodeInterval* result_interval;


	if (intervaltree_) {
		//result_interval = intervaltree_->Lookup(	
	}

}


int Inode::Write(char* src, uint64_t off, uint64_t n)
{
	uint64_t         tot;
	uint64_t         m;
	uint64_t         fbn; // first block number
	uint64_t         lbn; // last block number
	uint64_t         bn;
	PInode::Iterator start;
	PInode::Iterator iter;
	InodeInterval*   result_interval;

	
	fb = off/BLOCK_SIZE;
	lb = (off+n)/BLOCK_SIZE;
	
	bn = fbn;
	while (bn <= lbn) {
		//start.Init(pinode_, bn);
		result_interval = intervaltree_->LeftmostOverlap(bn, lbn);
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

