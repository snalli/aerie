#ifndef __STAMNOS_SAL_POOL_USER_H
#define __STAMNOS_SAL_POOL_USER_H

#include <stddef.h>
#include <stdint.h>

class PersistentRegion; // forward declaration
class DynamicBitSet;    // forward declaration

class StoragePool {
public:
	
	static int Create(const char* pathname, size_t size);
	static int Open(const char* pathname, StoragePool** pool);
	static int Close(StoragePool* pool);

	int AllocateExtent(uint64_t size, void** ptr);

private:
	struct Header;
	
	StoragePool(PersistentRegion* pregion, Header* header);

	PersistentRegion* pregion_;
	Header*           header_;
	DynamicBitSet*    bitset_;
	uint64_t          extents_base_;
};

#endif // __STAMNOS_SAL_POOL_USER_H
