#include "sal/pregion/pregion.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "common/errno.h"
#include "common/util.h"


uint64_t PersistentRegion::last_mmap_base_addr_ = kPersistentHoleLowBound;


int 
PersistentRegion::Header::Load(int fd, Header* header) {
	if (pread(fd, (void*) header, sizeof(*header), 0) < 0) {
		return -E_ERRNO;
	}
	return E_SUCCESS;
}


int 
PersistentRegion::Header::Store(int fd, Header& header) {
	if (pwrite(fd, (void*) &header, sizeof(header), 0) < 0) {
		return -E_ERRNO;
	}
	return E_SUCCESS;
}


/** 
 * Creates a region of size nblocks * block_size aligned at a multiple of 
 * align_size
 *
 * If successful, it returns the open file descriptor.
 *
 */
int
PersistentRegion::CreateInternal(const char* pathname, size_t nblocks, 
                                 size_t block_size, size_t align_size, int flags)
{
	int      fd;
	uint64_t size;
	size_t   align_multiple;
	int      normalized_flags = 0;

	align_multiple = (sizeof(PersistentRegion::Header) / align_size) + 
	                 (sizeof(PersistentRegion::Header) % align_size ? 1 : 0); 
	size = nblocks * block_size + align_multiple * align_size;
	normalized_flags = O_RDWR | O_CREAT | O_TRUNC;
	normalized_flags |= (flags & PersistentRegion::kExclusive) ? O_EXCL : 0;
	if ((fd = open(pathname, normalized_flags, 0666)) < 0) {
		return -E_ERRNO;
	}
	if (ftruncate(fd, size) < 0) {
		return -E_ERRNO;
	}
	
	PersistentRegion::Header header(align_multiple*align_size, nblocks*block_size);
	if (PersistentRegion::Header::Store(fd, header) < 0) {
		return -E_ERRNO;
	}
	return fd;
}


int
PersistentRegion::Create(const char* pathname, size_t size)
{
	size_t nblocks = NumOfBlocks(size, kBlockSize);
	int    ret;

	if ((ret = CreateInternal(pathname, nblocks, kBlockSize, kBlockSize,
	                          PersistentRegion::kCreate)) < 0) {
		return ret;
	}
	close(ret);
	return E_SUCCESS;
}


/**
 * \brief Opens and maps the persistent region stored in the file. If the
 * region has been mapped before then it is remapped to its previous location.
 * 
 * When called with the flags PersistentRegion::kCreate | PersistentRegion::kExclusive
 * it creates the region if the region doesn't exist.
 */
int 
PersistentRegion::Open(const char* pathname, size_t size, int flags, 
                       PersistentRegion** pregion) 
{
	int                       ret;
	int                       fd = -1;
	int                       mmap_flags;
	PersistentRegion::Header  header;
	uint64_t                  mmap_addr;
	void*                     base_addr;
	void*                     header_mmap_addr;

	*pregion = NULL;
	
	if (flags & PersistentRegion::kCreate) {
		if (flags & PersistentRegion::kExclusive) {
			size_t nblocks = NumOfBlocks(size, kBlockSize);
			if ((ret = PersistentRegion::CreateInternal(pathname, nblocks, kBlockSize, 
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
			if ((ret = PersistentRegion::CreateInternal(pathname, nblocks, kBlockSize, 
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
	if ((ret = PersistentRegion::Header::Load(fd, &header)) < 0) {
		goto done;
	}

	mmap_flags = MAP_SHARED;
	if (header.base_addr() == 0) {
		mmap_addr = last_mmap_base_addr_;
	} else {
		mmap_flags |= MAP_FIXED;
		mmap_addr = header.base_addr();
	}
	
	if ((base_addr = mmap((void*) mmap_addr, header.length(), PROT_WRITE | PROT_READ, 
	                       mmap_flags, fd, header.gap())) == (void *) -1) {
		ret = -E_ERRNO;
		goto done;
	}
	
	if ((header_mmap_addr = mmap((void*) 0, header.gap(), PROT_WRITE | PROT_READ, 
	                             MAP_SHARED, fd, 0)) == (void *) -1) {
		ret = -E_ERRNO;
		goto done;
	}
	
	// keep track the rightmost mmaped region to continue searching from there
	last_mmap_base_addr_ = (uint64_t) base_addr + header.length();

	*pregion = new PersistentRegion;
	(*pregion)->base_ = (uint64_t) base_addr;
	(*pregion)->length_ = (uint64_t) header.length();
	(*pregion)->header_ = (PersistentRegion::Header*) header_mmap_addr;
	(*pregion)->fd_ = fd;

	// store the mmapped address for reincarnation
	(*pregion)->header_->set_base_addr((uint64_t) base_addr);

	ret = E_SUCCESS;
done:
	if (flock(fd, LOCK_UN) < 0) {
		return -E_ERRNO;
	}
	return ret;
}


int 
PersistentRegion::Open(const char* pathname, PersistentRegion** pregion) 
{
	return Open(pathname, 0, 0, pregion);
}


int
PersistentRegion::Close(PersistentRegion* pregion)
{
	if (munmap((void*) pregion->base_, pregion->length_) < 0) {
		return -E_ERRNO;
	}
	if (munmap((void*) pregion->header_, pregion->header_->gap()) < 0) {
		return -E_ERRNO;
	}
	close(pregion->fd_);
	delete pregion;
	return E_SUCCESS;
}


int
PersistentRegion::Lock()
{
	if (flock(fd_, LOCK_EX) < 0) {
		return -E_ERRNO;
	}
	return E_SUCCESS;
}


int
PersistentRegion::Unlock()
{
	if (flock(fd_, LOCK_UN) < 0) {
		return -E_ERRNO;
	}
	return E_SUCCESS;
}
