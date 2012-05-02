#ifndef __STAMNOS_SPA_POOL_KERNEL_H
#define __STAMNOS_SPA_POOL_KERNEL_H

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <list>

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
	int FreeExtent(void* ptr);

	void set_root(void* root);
	void* root();

	void PrintStats();
//private:
	struct Header;
	
	StoragePool(Header* header);

	Header*    header_;
	uint64_t   identity_;
	size_t     alloc_size_;
	size_t     free_size_;
	std::list<void*>  free_list_;
};

#endif // __STAMNOS_SPA_POOL_KERNEL_H
