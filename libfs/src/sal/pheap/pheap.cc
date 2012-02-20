#include "sal/pheap/pheap.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "sal/pregion/pregion.h"
#include "sal/pheap/vistaheap.h"
#include "common/errno.h"
#include "common/util.h"
#include "common/debug.h"


int
PersistentHeap::Open(const char* filename, size_t size, PersistentHeap* allocatorp,
                     int flags, PersistentHeap** pheap)
{
	void*             vistaheap_base;
	void*             vistaheap_limit;
	VistaHeap*        vistaheapp;
	VistaHeap*        allocator;
	PersistentRegion* pregion;
	int               ret;
	int               region_flags;

	*pheap  = NULL;

	region_flags = PersistentRegion::kCreate;
	region_flags |= (flags & PersistentHeap::kReset) ? 0 : PersistentRegion::kExclusive;

	if ((ret = PersistentRegion::Open(filename, size, region_flags, &pregion)) < 0) {
		return ret;
	}
	PersistentHeap::Header* header = PersistentHeap::Header::Load(pregion->Payload());
	if (!header->initialized()) {
		header = PersistentHeap::Header::Make(pregion->Payload());
		vistaheapp = (VistaHeap*) pregion->base();
		vistaheap_base = (void *) ((uintptr_t) vistaheapp + sizeof(*vistaheapp));
		vistaheap_limit = (void *) ((uintptr_t) pregion->base() + pregion->Size());
		allocator = (allocatorp) ? allocatorp->vistaheap() : vistaheapp;
		vistaheap_init(vistaheapp, vistaheap_base, vistaheap_limit, allocator);
		header->set_vistaheap(vistaheapp);
	}

	*pheap = new PersistentHeap(pregion, header);

	return E_SUCCESS;
}


int
PersistentHeap::Close()
{
	return pregion_->Close();
}
