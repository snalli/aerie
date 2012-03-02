#ifndef __STAMNOS_SAL_POOL_USER_H
#define __STAMNOS_SAL_POOL_USER_H

#include <stddef.h>
#include <stdint.h>

class PersistentRegion; // forward declaration
class DynamicBitSet;    // forward declaration

class StoragePool {
public:
	
	static int Create(const char* path, size_t size);
	static int Open(const char* path, StoragePool** pool);
	static int Close(StoragePool* pool);
	static int Identity(const char* path, uint64_t* identity);
	uint64_t Identity() { return identity_; }

	int AllocateExtent(uint64_t size, void** ptr);

	void set_root(void* root);
	void* root();

private:
	struct Header;
	
	StoragePool(PersistentRegion* pregion, Header* header);

	PersistentRegion* pregion_;
	Header*           header_;
	DynamicBitSet*    bitset_;
	uint64_t          extents_base_;
	uint64_t          identity_;
};

#endif // __STAMNOS_SAL_POOL_USER_H
