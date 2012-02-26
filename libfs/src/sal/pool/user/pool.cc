/**
 * \brief A user-mode implementation of the pool abstraction.
 *
 * The pool is implemented on top of a persistent region. 
 */

#include "sal/pool/user/pool_i.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <errno.h>
#include "sal/pregion/pregion.h"
#include "sal/const.h"
#include "common/util.h"
#include "common/bitset.h"
#include "common/errno.h"


StoragePool::StoragePool(PersistentRegion* pregion, Header* header)
	: pregion_(pregion),
	  header_(header)
{ }


// persistent region must accomodate the pool's raw space and the pool's
// metadata:
// RAW SPACE: size
// METADATA : sizeof(bitset to track each page of RAW space)
int
StoragePool::Create(const char* path, size_t size)
{
	int               ret;
	int               region_flags;
	PersistentRegion* pregion;
	StoragePool*      pool;
	uint64_t          npages = NumOfBlocks(size, kBlockSize);
	uint64_t          bitset_size = DynamicBitSet::Sizeof(npages);
	uint64_t          bitset_npages = NumOfBlocks(bitset_size, kBlockSize);
	uint64_t          normalized_size = (npages + bitset_npages) * kBlockSize;
	
	region_flags = PersistentRegion::kCreate;
	if ((ret = PersistentRegion::Open(path, normalized_size, region_flags, &pregion)) < 0) {
		return ret;
	}
	if (DynamicBitSet::Make((void*) pregion->base(), bitset_size) == NULL) {
		return -E_NOMEM;
	}
	return PersistentRegion::Close(pregion);
}


int
StoragePool::Open(const char* path, StoragePool** pool)
{
	PersistentRegion* pregion;
	uint64_t          bitset_npages;
	int               ret;

	if ((ret = PersistentRegion::Open(path, &pregion)) < 0) {
		return ret;
	}
	StoragePool::Header* header = StoragePool::Header::Load(pregion->Payload());
	*pool = new StoragePool(pregion, header);
	DynamicBitSet* bitset = DynamicBitSet::Load((void*) pregion->base());
	(*pool)->bitset_ = bitset;
	bitset_npages = NumOfBlocks(bitset->Size(), kBlockSize);
	(*pool)->extents_base_ = pregion->base() + bitset_npages * kBlockSize;
	return E_SUCCESS;
}


// does a first-fit allocation (quick and dirty...)
int 
StoragePool::AllocateExtent(uint64_t size, void** ptr)
{
	int        start;
	uint64_t   nblocks = NumOfBlocks(size, kBlockSize);

	assert(pregion_->Lock() == E_SUCCESS);
	for (int i=0; i < bitset_->Size(); i++) {
		if (i + nblocks > bitset_->Size()) {
			// overflow; no way to found empty space
			assert(pregion_->Unlock() == E_SUCCESS);
			return -E_NOMEM;
		}
		// look for contiguous blocks
		bool found = true;
		for (int j=i; j < i + nblocks; j++) {
			if ((*bitset_)[j] == true) {
				found = false;
				break;
			}
		}
		if (found) {
			start = i;
			for (int j=i; j < i + nblocks; j++) {
				(*bitset_)[j] = true;
			}
			break;
		}
	}
	*ptr = (void*) (extents_base_ + start*kBlockSize);
	assert(pregion_->Unlock() == E_SUCCESS);
	return E_SUCCESS;
}


int 
StoragePool::Close(StoragePool* pool)
{
	int ret;
	if ((ret = PersistentRegion::Close(pool->pregion_)) < 0) {
		return ret;
	}
	delete pool;
	return E_SUCCESS;
}


/**
 * \brief Returns a unique 64-bit number identifying the storage pool
 * at path
 */
int
StoragePool::Identity(const char* path, uint64_t* identity) 
{
	int         ret;
	struct stat buf;
	
	if ((ret = stat(path, &buf)) < 0) {
		printf("%s\n", strerror(errno));
		return -E_ERRNO;
	}
	*identity = buf.st_ino;
	return E_SUCCESS;
}


void 
StoragePool::set_root(void* root)
{
	header_->set_root(root);
}


void* 
StoragePool::root()
{
	return header_->root();
}
