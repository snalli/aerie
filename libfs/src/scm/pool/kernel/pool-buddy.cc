/**
 * \brief Pool allocator abstraction on top of kernel-mode interface.
 *
 * The pool is implemented on top of a persistent region. 
 */

#include "scm/pool/kernel/pool_i-buddy.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>
#include <syscall.h>
#include <errno.h>
#include <pthread.h>
#include "bcs/main/common/debug.h"
#include "scm/pregion/pregion.h"
#include "scm/const.h"
#include "common/util.h"
#include "common/bitset.h"
#include "common/errno.h"
#include "common/prof.h"

//#define PROFILER_SAMPLE __PROFILER_SAMPLE



typedef struct {
    uid_t uid;
    int rw;
} user_file_rights;

typedef struct {
	unsigned long base;
	unsigned long size;
} KernelExtent;


StoragePool::StoragePool(Header* header)
	: header_(header)
{ }


int 
StoragePool::Allocate(const char* path, size_t size)
{
	unsigned long v_addr;
	unsigned long size_mb;

	v_addr = 0x8000000000;
	//size_mb = 1024;
	size_mb = 8192;

	// convention: syscall 313 returns 1 on success
	if (syscall(312, v_addr, size_mb) != 1) {
		return -E_PROT;
	}
	return E_SUCCESS;
}


int
StoragePool::Create(const char* path, size_t size, int flags)
{
	int           ret;
	int           fd;
	size_t        bitmap_size;
	size_t        header_size;
	size_t        metadata_size;
	size_t        freelist_size;
	size_t        num_blocks;
	unsigned long base;
	unsigned long bitmap_addr;
	unsigned long freelist_addr;
	unsigned long header_addr;

	if (size < 1024*1024*1024 /* 1GB */) {
		size = 1024*1024*1024;
	}
	// we create a file to ensure we don't do multiple allocations
	if ((fd = open(path, O_CREAT|O_TRUNC|O_EXCL, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) > 0) {
		Allocate(path, size);
		close(fd);
	}

	header_size = sizeof(StoragePool::Header);
	bitmap_size = poolBitMapSize(size, 12); // 12 bits for 4K page size
	freelist_size = sizeof(size_t) * 32;

	base = 0x8000000000;
	header_addr = base;
	freelist_addr = header_addr + header_size;
	bitmap_addr = freelist_addr + freelist_size;
	metadata_size = header_size + freelist_size + bitmap_size;
	metadata_size = NumOfBlocks(metadata_size, kBlockSize) * kBlockSize;
	if ((ret = Protect(base, size, getuid(), 0x3)) < 0) {
		return ret;
	}
	if (size < metadata_size) {
		return -E_INVAL;
	}
	num_blocks = (size - metadata_size) >> 12;
	StoragePool::Header* header = StoragePool::Header::Make((void*) header_addr);
	if ((poolInit((void*) (base + metadata_size), size - metadata_size, 12, 32, 
	         (size_t*) freelist_addr, (char*) bitmap_addr, &header->buddy_)) != NULL) {
		return -E_NOMEM;
	}
	if (poolRelease(&header->buddy_, 0, num_blocks) != NULL) {
		return -E_NOMEM;
	}
	return E_SUCCESS;
}


int
StoragePool::Protect(unsigned long extent_base, size_t extent_size, uid_t uid, int rw)
{
	int              ret;
	KernelExtent     e;
	user_file_rights r;

	e.base = extent_base;
	e.size = extent_size; // convention: size in bytes
	r.uid = uid;
	r.rw = rw;

	// convention: syscall 313 returns 1 on success
	if (syscall(313, (void*) &e, (void*) &r, 1) != 1) {
		return -E_PROT;
	}
	return E_SUCCESS;
}


int
StoragePool::Open(const char* path, StoragePool** pool)
{
	unsigned long  header_addr;
	unsigned long  base;

	base = 0x8000000000;
	header_addr = base;
	StoragePool::Header* header = StoragePool::Header::Load((void*)header_addr);
	*pool = new StoragePool(header);
	(*pool)->free_size_ = 0;
	(*pool)->alloc_size_ = 0;
	return StoragePool::Identity(path, &((*pool)->identity_));
}


int 
StoragePool::AllocateExtent(uint64_t size, void** ptr)
{
	PROFILER_PREAMBLE
	int           ret;
	unsigned long extent_base;
	PROFILER_SAMPLE
	
	//FIXME: buddy is not really a buddy. if we try to return a page  and 
	//not the whole region it will leak storage. so get from frees here
	if (size == 4096 && !free_list_.empty()) {
		*ptr = free_list_.front();
		free_list_.pop_front();
		return E_SUCCESS;
	}
	// roundup because protect expects multiple page size
	size = NumOfBlocks(size, kBlockSize) * kBlockSize; 

	if (poolMalloc(&header_->buddy_, size, ptr) != NULL) {
		return -E_NOMEM;
	}
	alloc_size_+=size;
	PROFILER_SAMPLE
	extent_base = (unsigned long) *ptr;
//FIXME: Enable protection change below
#if 0 
	if ((ret = Protect(extent_base, size, getuid(), 0x3)) < 0) {
		printf("failed protection: %p\n", *ptr);
		return ret;
	}
#endif
	PROFILER_SAMPLE
	return E_SUCCESS;
}


int 
StoragePool::FreeExtent(void* ptr)
{
	int           ret;
	unsigned long extent_base;
	
	free_size_ += 4096;
	
	//FIXME: buddy is not really a buddy. if we try to return a page  and 
	//not the whole region it will leak storage. so track frees here
	free_list_.push_back(ptr);
	return E_SUCCESS;

	if (poolFree(&header_->buddy_, ptr) != NULL) {
		return -E_ERROR;
	}
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

void
StoragePool::PrintStats() {
	printf("ALLOCATED: %lu\n", alloc_size_);
	printf("FREED: %lu\n", free_size_);
	printf("FREELIST: %lu\n", free_list_.size());
}
