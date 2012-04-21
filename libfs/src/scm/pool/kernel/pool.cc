/**
 * \brief Pool allocator abstraction on top of kernel-mode interface.
 *
 * The pool is implemented on top of a persistent region. 
 */

#include "scm/pool/kernel/pool_i.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <syscall.h>
#include <errno.h>
#include "scm/pregion/pregion.h"
#include "scm/const.h"
#include "common/util.h"
#include "common/bitset.h"
#include "common/errno.h"


StoragePool::StoragePool(Header* header)
	: header_(header)
{ }


int 
StoragePool::Allocate(const char* path, size_t size)
{
	unsigned long v_addr;
	unsigned long size_mb;

	v_addr = 0x8000000000;
	size_mb = 1024;

	return syscall(312, v_addr, size_mb);
}


// persistent region must accomodate the pool's raw space and the pool's
// metadata:
// RAW SPACE: size
// METADATA : sizeof(bitset to track each page of RAW space)
int
StoragePool::Create(const char* path, size_t size, int flags)
{
	int    fd;
	size_t        bitmap_size;
	size_t        header_size;
	size_t        metadata_size;
	size_t        freelist_size;
	unsigned long base;
	unsigned long bitmap_addr;
	unsigned long freelist_addr;
	unsigned long header_addr;

	// we create a file to ensure we don't do multiple allocations
	if ((fd = open(path, O_CREAT|O_TRUNC|O_EXCL, S_IRUSR|S_IWUSR)) > 0) {
		Allocate(path, size);
		close(fd);
	}

	header_size = sizeof(StoragePool::Header);
	bitmap_size = poolBitMapSize(size, 12);
	freelist_size = sizeof(size_t) * 32;

	base = 0x8000000000;
	header_addr = base;
	freelist_addr = header_addr + header_size;
	bitmap_addr = freelist_addr + freelist_size;
	metadata_size = header_size + freelist_size + bitmap_size;
	metadata_size = NumOfBlocks(metadata_size, kBlockSize) * kBlockSize;
	printf("%lu\n", metadata_size);
	//Protect(base, metadata_size, 0, 0x11);
	StoragePool::Header* header = StoragePool::Header::Make((void*) header_addr);
	//poolInit((void*) (base + metadata_size), size - metadata_size, 12, 32, 
	//         (size_t*) freelist_addr, (char*) bitmap_addr, &header->buddy_);
	return E_SUCCESS;
}

typedef struct {
    uid_t uid;
    int rw;
} user_file_rights;

typedef struct {
	unsigned long base;
	unsigned long size;
} KernelExtent;


int
StoragePool::Protect(unsigned long extent_base, size_t extent_size, uid_t uid, int rw)
{
	KernelExtent     e;
	user_file_rights r;

	e.base = extent_base;
	e.size = extent_size;
	r.uid = uid;
	r.rw = rw;

	syscall(313, (void*) &e, (void*) &r, 1);
	return E_SUCCESS;
}




int
StoragePool::Open(const char* path, StoragePool** pool)
{
	PersistentRegion* pregion;
	uint64_t          bitset_npages;
	int               ret;

	return StoragePool::Identity(path, &((*pool)->identity_));
}


// does a first-fit allocation (quick and dirty...)
int 
StoragePool::AllocateExtent(uint64_t size, void** ptr)
{
	return E_SUCCESS;
}


int 
StoragePool::Close(StoragePool* pool)
{
	return E_SUCCESS;
}


/**
 * \brief Returns a unique 64-bit number identifying the storage pool
 * at path
 */
int
StoragePool::Identity(const char* path, uint64_t* identity) 
{
	*identity = 0x8;
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
