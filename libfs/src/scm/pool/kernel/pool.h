#ifndef __STAMNOS_SPA_POOL_KERNEL_H
#define __STAMNOS_SPA_POOL_KERNEL_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

class PersistentRegion; // forward declaration
class DynamicBitSet;    // forward declaration

class StoragePool {
public:
	
	static int Allocate(const char* path, size_t size);
	static int Protect(unsigned long extent_base, size_t extent_size, uid_t uid, int rw);
	static int Create(const char* path, size_t size, int flags);
	static int Open(const char* path, StoragePool** pool);
	static int Close(StoragePool* pool);
	static int Identity(const char* path, uint64_t* identity);
	uint64_t Identity() { return identity_; }

	int AllocateExtent(uint64_t size, void** ptr);

	void set_root(void* root);
	void* root();

//private:
	struct Header;
	
	StoragePool(Header* header);

	PersistentRegion* pregion_;
	Header*           header_;
	uint64_t          extents_base_;
	uint64_t          identity_;
};

#endif // __STAMNOS_SPA_POOL_KERNEL_H
