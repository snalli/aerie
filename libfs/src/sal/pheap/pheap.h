#ifndef __STAMNOS_SAL_PERSISTENT_HEAP_H
#define __STAMNOS_SAL_PERSISTENT_HEAP_H

#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include "sal/pheap/vistaheap.h"


class PersistentHeap {
public:
	struct Header;

	static int Open(const char* filename, size_t maxsize,
					PersistentHeap* allocatorp, PersistentHeap** pheap);
	int Map(const char* filename, size_t maxsize, int prot_flags);
	int Close();
	//inline int Alloc(size_t size, void **pp);
	//inline void Free(void *p, size_t size);
	inline VistaHeap* vistaheap();

private:
	VistaHeap* vistaheap_;
	void*      root_;
};

/*
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

*/

VistaHeap*
PersistentHeap::vistaheap()
{
	return vistaheap_;
}

#endif // __STAMNOS_SAL_PERSISTENT_HEAP_H
