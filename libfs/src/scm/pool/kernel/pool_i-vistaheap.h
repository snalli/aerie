#ifndef __STAMNOS_SPA_POOL_KERNEL_INTERNAL_H
#define __STAMNOS_SPA_POOL_KERNEL_INTERNAL_H

#include "scm/pool/kernel/pool.h"
#include "scm/pheap/vistaheap.h"


struct StoragePool::Header {
friend class StoragePool;
public:	
	static Header* Load(void* b) {
		return reinterpret_cast<Header*>(b);
	}

	static Header* Make(void* b) {
		Header* header = reinterpret_cast<Header*>(b);
		header->root_ = 0;
		return header;
	}

	void* root() { return root_; }
	void set_root(void* root) { root_ = root; }

//private:
	void*       root_;
	VistaHeap   vistaheap_;
};



#endif // __STAMNOS_SPA_POOL_USER_INTERNAL_H
