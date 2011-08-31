#ifndef _PHEAP_H_TAH111
#define _PHEAP_H_TAH111

#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include "common/vistaheap.h"

enum {
	PHEAP_INITIALIZED = 0x00000001
};

struct PHeapHeader {
	uint32_t magic;
	void*    mmap_base;
	uint64_t mmap_length;
};
typedef struct PHeapHeader PHeapHeader;

class PHeap {
	public:
		PHeap();
		int Open(char* filename, size_t maxsize, size_t root_size,
                 size_t align, PHeap* allocatorp);
		int Map(char* filename, size_t maxsize, int prot_flags);
		inline int Alloc(size_t size, void **pp);
		inline void Free(void *p, size_t size);
		inline vistaheap *get_vistaheap();
		inline void *get_root();
		int Close();

	private:
		vistaheap* _vistaheap;
		void*      _root;
};


int 
PHeap::Alloc(size_t size, void **pp) 
{
	void *p;
	if (!(p = vistaheap_malloc(_vistaheap, size))) { 
		return -1;
	}
	*pp = p;
	return 0;
}


void 
PHeap::Free(void *p, size_t size) 
{
	vistaheap_free(_vistaheap, p, size);
}


void*
PHeap::get_root()
{
	return _root;
}


vistaheap*
PHeap::get_vistaheap()
{
	return _vistaheap;
}

#endif
