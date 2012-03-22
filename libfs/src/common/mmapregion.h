/**
 * \brief Memory Mapped Region 
 */

#ifndef __STAMNOS_COMMON_MEMORY_MAPPED_BUFFER_H
#define __STAMNOS_COMMON_MEMORY_MAPPED_BUFFER_H

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "spa/const.h"
#include "common/errno.h"
#include "common/util.h"



template<typename HeaderT>
class MemoryMappedRegion {
public:
	enum Flags {
		kCreate = 0x1,      /**< Creates the file backing the region */
		kExclusive = 0x2,   /**< When combined with kCreate, the region is only created if it doesn't already exist */
		kMap = 0x4,         /**< Maps the region */
		kMapPrevious = 0x8, /**< Maps the region to the address region is was previously mapped */
	};
	
	class Header;
	
	static int Create(const char* pathname, size_t size);
	static int Open(const char* pathname, size_t size, int flags, MemoryMappedRegion* region);
	static int Open(const char* pathname, size_t size, int flags, MemoryMappedRegion** region);
	static int Open(const char* pathname, MemoryMappedRegion** region);
	static int Close(MemoryMappedRegion* region);

	uint64_t base() { return base_; }
	uint64_t Size() { return size_; }
	int Lock();
	int Unlock();

protected:
	static int CreateInternal(const char* pathname, size_t nblocks, size_t block_size, size_t align_size, int flags);

	int      fd_;
	uint64_t base_;
	uint64_t size_;
	Header*  header_; // where the header is memory mmapped
};


/** 
 * Creates a region of size nblocks * block_size aligned at a multiple of 
 * align_size
 *
 * If successful, it returns the open file descriptor.
 *
 */
template<typename HeaderT>
int
MemoryMappedRegion<HeaderT>::CreateInternal(const char* pathname, size_t nblocks, 
                                            size_t block_size, size_t align_size, int flags)
{
	int      fd;
	uint64_t size;
	size_t   align_multiple;
	int      normalized_flags = 0;

	align_multiple = (sizeof(MemoryMappedRegion::Header) / align_size) + 
	                 (sizeof(MemoryMappedRegion::Header) % align_size ? 1 : 0); 
	size = nblocks * block_size + align_multiple * align_size;
	normalized_flags = O_RDWR | O_CREAT | O_TRUNC;
	normalized_flags |= (flags & MemoryMappedRegion::kExclusive) ? O_EXCL : 0;
	if ((fd = open(pathname, normalized_flags, 0666)) < 0) {
		return -E_ERRNO;
	}
	if (ftruncate(fd, size) < 0) {
		return -E_ERRNO;
	}
	
	MemoryMappedRegion::Header header(align_multiple*align_size, nblocks*block_size);
	if (MemoryMappedRegion::Header::Store(fd, header) < 0) {
		return -E_ERRNO;
	}
	return fd;
}


template<typename HeaderT>
int
MemoryMappedRegion<HeaderT>::Create(const char* pathname, size_t size)
{
	size_t nblocks = NumOfBlocks(size, kBlockSize);
	int    ret;

	if ((ret = CreateInternal(pathname, nblocks, kBlockSize, kBlockSize,
	                          MemoryMappedRegion::kCreate)) < 0) {
		return ret;
	}
	close(ret);
	return E_SUCCESS;
}


/**
 * \brief Opens and maps the persistent region stored in the file. If the
 * region has been mapped before then it is remapped to its previous location.
 * 
 * When called with the flags MemoryMappedRegion::kCreate | MemoryMappedRegion::kExclusive
 * it creates the region if the region doesn't exist.
 */
template<typename HeaderT>
int 
MemoryMappedRegion<HeaderT>::Open(const char* pathname, size_t size, int flags, 
                                  MemoryMappedRegion* region) 
{
	int                        ret;
	int                        fd = -1;
	int                        mmap_flags;
	MemoryMappedRegion::Header header;
	uint64_t                   mmap_addr;
	void*                      base_addr;
	void*                      header_mmap_addr;

	if (flags & MemoryMappedRegion::kCreate) {
		if (flags & MemoryMappedRegion::kExclusive) {
			size_t nblocks = NumOfBlocks(size, kBlockSize);
			if ((ret = MemoryMappedRegion::CreateInternal(pathname, nblocks, kBlockSize, 
                                                          kBlockSize, flags)) < 0) {
				if (ret == -E_ERRNO && errno == EEXIST) {
					// fall through: we open the existing file
				} else {
					return ret;
				}
			}
			fd = ret;
		} else {
			size_t nblocks = NumOfBlocks(size, kBlockSize);
			if ((ret = MemoryMappedRegion::CreateInternal(pathname, nblocks, kBlockSize, 
                                                          kBlockSize, flags)) < 0) {
				return ret;
			}
			fd = ret;
		}
	}

	// Proceed with opening the region.
	// if we couldn't create the region then the region might already exist or we
	// might have lost the race against another thread/process trying to create 
	// the region. If that's the case then the region should already exist so
	// we fall through and we try to open it.
	if (fd < 0) {
		if ((fd = open(pathname, O_RDWR)) < 0) {
			return -E_ERRNO;
		}
	}

	if (flock(fd, LOCK_EX) < 0) {
		close(fd);
		return -E_ERRNO;
	}
	if ((ret = MemoryMappedRegion::Header::Load(fd, &header)) < 0) {
		goto done;
	}

	mmap_flags = MAP_SHARED;
	if (flags & kMapPrevious && header.payload_base() != 0) {
		mmap_flags |= MAP_FIXED;
		mmap_addr = header.payload_base();
	} else {
		mmap_addr = 0;
	}
	
	if ((base_addr = mmap((void*) mmap_addr, header.payload_size(), PROT_WRITE | PROT_READ, 
	                       mmap_flags, fd, header.header_size())) == (void *) -1) {
		ret = -E_ERRNO;
		goto done;
	}
	
	if ((header_mmap_addr = mmap((void*) 0, header.header_size(), PROT_WRITE | PROT_READ, 
	                             MAP_SHARED, fd, 0)) == (void *) -1) {
		ret = -E_ERRNO;
		goto done;
	}
	
	region = new MemoryMappedRegion;
	region->base_ = (uint64_t) base_addr;
	region->size_ = (uint64_t) header.payload_size();
	region->header_ = (MemoryMappedRegion::Header*) header_mmap_addr;
	region->fd_ = fd;

	// store the mmapped address for reincarnation
	region->header_->set_payload_base((uint64_t) base_addr);

	ret = E_SUCCESS;
done:
	if (flock(fd, LOCK_UN) < 0) {
		return -E_ERRNO;
	}
	return ret;
}


template<typename HeaderT>
int 
MemoryMappedRegion<HeaderT>::Open(const char* pathname, size_t size, int flags, 
                                  MemoryMappedRegion** regionp) 
{
	int ret;
	MemoryMappedRegion region = new MemoryMappedRegion;
	if ((ret = Open(pathname, size, flags, region)) < 0) {
		delete region;
		return ret;
	}
	*regionp = region;
	return ret;
}


template<typename HeaderT>
int 
MemoryMappedRegion<HeaderT>::Open(const char* pathname, MemoryMappedRegion** region) 
{
	return Open(pathname, 0, 0, region);
}


template<typename HeaderT>
int
MemoryMappedRegion<HeaderT>::Close(MemoryMappedRegion* region)
{
	if (munmap((void*) region->base_, region->size_) < 0) {
		return -E_ERRNO;
	}
	if (munmap((void*) region->header_, region->header_->header_size()) < 0) {
		return -E_ERRNO;
	}
	close(region->fd_);
	delete region;
	return E_SUCCESS;
}


template<typename HeaderT>
int
MemoryMappedRegion<HeaderT>::Lock()
{
	if (flock(fd_, LOCK_EX) < 0) {
		return -E_ERRNO;
	}
	return E_SUCCESS;
}


template<typename HeaderT>
int
MemoryMappedRegion<HeaderT>::Unlock()
{
	if (flock(fd_, LOCK_UN) < 0) {
		return -E_ERRNO;
	}
	return E_SUCCESS;
}



/*
 * MEMORY MAPPED REGION HEADER 
 */

template<typename HeaderT>
class MemoryMappedRegion<HeaderT>::Header: public HeaderT {
public:
	Header()
		: initialized_(false),
		  header_size_(0),
		  payload_base_(0),
		  payload_size_(0)
	{ }

	Header(uint64_t header_size, uint64_t payload_size)
		: initialized_(true),
		  header_size_(header_size),
		  payload_base_(0),
		  payload_size_(payload_size)
	{ }

	static int Load(int fd, Header* header);
	static int Store(int fd, Header& header);
	
	uint64_t payload_size() { return payload_size_; }
	uint64_t header_size() { return header_size_; }
	uint64_t payload_base() { return payload_base_; }
	void set_payload_base(uint64_t payload_base) { payload_base_ = payload_base; }
	bool initialized() { return initialized_; }

private:
	bool     initialized_;
	uint64_t header_size_;  // length of the space occupied by the header and extra space to properly align the region
	uint64_t payload_base_; // the address where the region payload was mapped (used by persistent region)
	uint64_t payload_size_;
};



template<typename HeaderT>
int 
MemoryMappedRegion<HeaderT>::Header::Load(int fd, Header* header) {
	if (pread(fd, (void*) header, sizeof(*header), 0) < 0) {
		return -E_ERRNO;
	}
	return E_SUCCESS;
}


template<typename HeaderT>
int 
MemoryMappedRegion<HeaderT>::Header::Store(int fd, Header& header) {
	if (pwrite(fd, (void*) &header, sizeof(header), 0) < 0) {
		return -E_ERRNO;
	}
	return E_SUCCESS;
}



#endif // __STAMNOS_COMMON_MEMORY_MAPPED_BUFFER_H
