#ifndef __STAMNOS_SAL_PERSISTENT_REGION_H
#define __STAMNOS_SAL_PERSISTENT_REGION_H

#include <stddef.h>
#include <stdint.h>

/**
 * We reserve a hole for persistent regions.
 * We must be careful with choosing the hole boundaries. The hole must not 
 * overlap with any other regions mapped with mmap such as dynamically loaded 
 * shared libraries. 
 *
 * Based on our experience the following hole seems to be not causing any 
 * overlapping problems:
 *
 * [0x0000100000000000LLU, 0x0000100000000000LLU + 0x0000010000000000LLU)
 *
 */
const uint64_t  kPersistentHoleLowBound = 0x0000100000000000LLU;
const uint64_t  kPersistentHoleSize     = 0x0000010000000000LLU; /* 1 TB */

const int kBlockShift = 12;
const int kBlockSize  = (1 << kBlockShift);


class PersistentRegion {
public:
	enum Flags {
		kCreate = 1,
		kExclusive = 2
	};
	
	struct Header;
	
	static int Create(const char* pathname, size_t size);
	static int Open(const char* pathname, size_t size, int flags, PersistentRegion** pregion);
	static int Open(const char* pathname, PersistentRegion** pregion);
	
	int Close();
	inline void* Payload();
	inline size_t PayloadMaxSize();

	uint64_t base() { return base_; }
	uint64_t length() { return length_; }
	uint64_t Size() { return length_; }

private:
	static int CreateInternal(const char* pathname, size_t nblocks, size_t block_size, size_t align_size, int flags);

	uint64_t base_;
	uint64_t length_;
	Header*  header_; // where header is memory mmapped

	// We want all mmaps of persistent regions to fall into the reserved hole.
	// If mmap is given a base address that overlaps with an already mmaped 
	// region then mmap's search algorithm searches outside the reserved hole. 
	// To avoid this problem we keep track the rightmost mmaped address to
	// continue searching from there.
	static uint64_t last_mmap_base_addr_;
};


// the persistent header kept at the head of the region.
// contains enough information to reincarnate it.
struct PersistentRegion::Header {
public:
	Header()
		: initialized_(false),
		  base_addr_(0),
		  gap_(0),
		  length_(0)
	{ }

	Header(uint64_t gap, uint64_t length)
		: initialized_(true),
		  base_addr_(0),
		  length_(length),
		  gap_(gap)
	{ }

	static int Load(int fd, Header* header);
	static int Store(int fd, Header& header);
	
	uint64_t length() { return length_; }
	uint64_t gap() { return gap_; }
	uint64_t base_addr() { return base_addr_; }
	void set_base_addr(uint64_t base_addr) { base_addr_ = base_addr; }
	bool initialized() { return initialized_; }
	uint64_t PayloadMaxSize() { return gap_ - sizeof(PersistentRegion::Header); }
	void* Payload() { return payload_; }

private:
	bool     initialized_;
	uint64_t base_addr_;
	uint64_t gap_; // length of the space occupied by the header and extra space to properly align the region
	uint64_t length_;
	char     payload_[0];
};



inline void* PersistentRegion::Payload() { 
	return header_->Payload(); 
}


inline size_t PersistentRegion::PayloadMaxSize() { 
	return header_->PayloadMaxSize(); 
}


#endif // __STAMNOS_SAL_PERSISTENT_REGION_H
