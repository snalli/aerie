#include "sal/pheap/pheap.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "sal/pheap/vistaheap.h"
#include "common/errno.h"
#include "common/util.h"
#include "common/debug.h"


int
PersistentHeap::Open(const char* filename, size_t size, PersistentHeap* allocatorp,
                     PersistentHeap** pheap)
{
	int          do_vista_init = 0;
	int          vistaheap_fd;
	void*        base_addr;
	void*        rootp;
	void*        vistaheap_base;
	void*        vistaheap_limit;
	VistaHeap*   vistaheapp;
	//PHeapHeader  pheapheader;
	//PHeapHeader* pheapheaderp;
	//vistaheap*   allocator;

/*
	if (!filename) {
		return -1;
	}	
	rounded_size = num_pages(maxsize) * kPageSize;
	mmap_flags = MAP_SHARED;
	if ((vistaheap_fd = open(filename, O_RDWR)) < 0) {
		vistaheap_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
		ftruncate(vistaheap_fd, rounded_size);
		do_vista_init = 1;
		mmap_addr = (void*) last_mmap_base_addr_;
		vistaheap_fd = open(filename, O_RDWR);
	} else {
		pread(vistaheap_fd, &pheapheader, sizeof(pheapheader), 0); 
		if (pheapheader.magic == PHEAP_INITIALIZED) {
			mmap_flags |= MAP_FIXED;
			mmap_addr = (void *) pheapheader.mmap_base;
		} else {
			do_vista_init = 1;
		}
	}
	if ((base_addr = mmap(mmap_addr, maxsize, PROT_WRITE | PROT_READ, mmap_flags, vistaheap_fd, 0)) == (void *) -1) {
		close(vistaheap_fd);
		if (do_vista_init) {
			unlink(filename);
		}
		return NULL;
	}
	last_mmap_base_addr_ = (void*) ( ((uint64_t) base_addr) + maxsize);
*/	
	

/*
	pheapheaderp = (PHeapHeader*) ((uintptr_t) base_addr);
	rootp = (void*) ((uintptr_t) pheapheaderp + sizeof(*pheapheaderp));
	vistaheapp = (vistaheap*) ((uintptr_t) rootp + root_size);
	vistaheap_base = (void *) ((uintptr_t) vistaheapp + sizeof(*vistaheapp));
	if (align > 1 && ((uintptr_t) vistaheap_base % align) != 0) {
		vistaheap_base = (void *) ((1 + ((uintptr_t) vistaheap_base)/align)*align);
	}
	vistaheap_limit = (void *) ((uintptr_t) base_addr + maxsize);
	assert((uintptr_t) vistaheap_base < (uintptr_t) vistaheap_limit);
	if (do_vista_init) {
		allocator = (allocatorp) ? allocatorp->get_vistaheap() : vistaheapp;
		vistaheap_init(vistaheapp, vistaheap_base, vistaheap_limit, allocator, vistaheap_fd);
		memset(rootp, 0, root_size);
		pheapheaderp->mmap_base = base_addr;
		pheapheaderp->magic = PHEAP_INITIALIZED;
	}
	_root = rootp; 
	_vistaheap = vistaheapp;
*/
	return E_SUCCESS;
}


int
PersistentHeap::Map(const char* filename, size_t maxsize, int prot_flags)
{
/*
	int          vistaheap_fd;
	int          mmap_flags;
	size_t       rounded_size;
	void*        base_addr;
	void*        mmap_addr;
	PHeapHeader  pheapheader;


	if (!filename) {
		return -1;
	}	
	rounded_size = num_pages(maxsize) * kPageSize;
	mmap_flags = MAP_SHARED;
	if ((vistaheap_fd = open(filename, O_RDWR)) < 0) {
		return -1;
	} else {
		pread(vistaheap_fd, &pheapheader, sizeof(pheapheader), 0); 
		if (pheapheader.magic == PHEAP_INITIALIZED) {
			mmap_flags |= MAP_FIXED;
			mmap_addr = (void *) pheapheader.mmap_base;
		} else {
			return -1;
		}
	}

	// FIXME: When using MAP_FIXED, the RPC library seg faults because the memory 
	// region specified by addr and len overlaps pages of existing mapping(s),
	// including RPC library mappings, which are discarded. The current workaround
	// is to use a small region to avoid overlapping. Since this functionality
	// will be implemented by the OS, we don't worry for now. 
	if ((base_addr = mmap(mmap_addr, maxsize, prot_flags, mmap_flags, vistaheap_fd, 0)) == (void *) -1) {
		dbg_log (DBG_WARNING, "mmap failed\n");
		close(vistaheap_fd);
		return NULL;
	}

	if (base_addr != mmap_addr) {
		dbg_log (DBG_ERROR, "mmap didn't map the persistent heap at the address requested\n");
	}

	_root = (void*) ((uintptr_t) base_addr + sizeof(pheapheader));
*/
	return 0;
}


int
PersistentHeap::Close()
{
	return E_SUCCESS;
}
