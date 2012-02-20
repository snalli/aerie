#ifndef __STAMNOS_SAL_PERSISTENT_HEAP_H
#define __STAMNOS_SAL_PERSISTENT_HEAP_H

#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include "sal/pheap/vistaheap.h"
#include "common/errno.h"

class PersistentRegion;  // forward declaration

class PersistentHeap {
public:
	struct Header;

	enum Flags {
		kReset = 1
	};

	PersistentHeap(PersistentRegion* pregion, Header* header)
		: header_(header),
		  pregion_(pregion)
	{ }

	static int Open(const char* filename, size_t maxsize,
					PersistentHeap* allocatorp, int flags, PersistentHeap** pheap);
	int Map(const char* filename, size_t maxsize, int prot_flags);
	int Close();
	inline int Alloc(size_t size, void **pp);
	inline void Free(void *p, size_t size);
	inline VistaHeap* vistaheap(); 
	inline void* root();
	inline void set_root(void* root);

private:
	PersistentRegion* pregion_;
	Header*           header_;
};


struct PersistentHeap::Header {
public:
	static Header* Load(void* b) {
		return reinterpret_cast<Header*>(b);
	}

	static Header* Make(void* b) {
		Header* header = reinterpret_cast<Header*>(b);
		header->root_ = 0;
		header->initialized_ = true;
		return header;
	}

	void* root() { return root_; }
	void set_root(void* root) { root_ = root; }
	bool initialized() { return initialized_; }
	void set_initialized(bool val) { initialized_ = val; }
	VistaHeap* vistaheap() { return vistaheap_; }
	void set_vistaheap(VistaHeap* vhp) { vistaheap_ = vhp; }

private:	
	bool       initialized_;
	void*      root_;
	VistaHeap* vistaheap_;
};


int 
PersistentHeap::Alloc(size_t size, void **pp) 
{
	void *p;
	if (!(p = vistaheap_malloc(vistaheap(), size))) { 
		return -E_NOMEM;
	}
	*pp = p;
	return E_SUCCESS;
}


void 
PersistentHeap::Free(void *p, size_t size) 
{
	vistaheap_free(vistaheap(), p, size);
}


inline VistaHeap* PersistentHeap::vistaheap() 
{ 
	return header_->vistaheap(); 
}


inline void* PersistentHeap::root() 
{ 
	return header_->root(); 
}


inline void PersistentHeap::set_root(void* root) 
{ 
	return header_->set_root(root); 
}


#endif // __STAMNOS_SAL_PERSISTENT_HEAP_H
