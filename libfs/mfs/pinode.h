#ifndef __PINODE_H_AKE111
#define __PINODE_H_AKE111

#include <pthread.h>
#include <stdint.h>
#include "mfs/mfs_i.h"
#include "mfs/radixtree.h"

const int N_DIRECT = 8;

class PInode 
{
public:
	PInode();
	
	// LookupSlot
	// LinkBlock2Slot
	// CompareAndSwapBlock? Verifies, and Links new block to slot and returns old block
	// SwapBlock? Links new block to slot and returns old block
	uint64_t LookupBlock(uint64_t bn);
	int LookupBlockRange(uint64_t bn, uint64_t n); // this returns a pointer to array of contiguous slots
	//SwapBlock
	int InsertBlock(void*, uint64_t, int);

	// Helper functions (mostly for testing functionality)
	int ReadBlock(uint64_t bn, char* dst, int n);
	int WriteBlock(uint64_t bn, char* src, int n);

	inline uint64_t get_size() {
		return size_;
	}
private:
	uint64_t  size_;
	uint64_t  daddrs_[N_DIRECT];
	RadixTree radixtree_;

};

#endif // __PINODE_H_AKE111
