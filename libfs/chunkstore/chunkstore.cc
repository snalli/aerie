#include "chunkstore.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
//#include "common/pheap.h"
#include "common/util.h"
#include "common/vistaheap.h"
#include "chunkdsc.h"


const uint64_t kChunkStoreSize = 1024*1024*1024;
const char*    kChunkStoreName = "chunkstore.vistaheap";

ChunkStore::ChunkStore(id_t principal_id):
	_principal_id(principal_id)
{
	
}


void 
ChunkStore::Init()
{
	int        ret;
	PHeap*     pheap;

	_lockspace = new LockSpace();
	ret = _lockspace->Init("chunkstore.locks", LS_VOLATILE, 1);

	_lockspace->Lock(0);
	
	_pheap = new PHeap();
	_pagepheap = new PHeap();
	_pheap->Open("chunkstore.pheap", 1024*1024, 0, 0, 0);
	_pagepheap->Open("chunkstore.pagepheap", kChunkStoreSize, 0, kPageSize, _pheap);

	_lockspace->Unlock(0);
}


int
ChunkStore::CreateChunk(size_t size, ChunkDescriptor **chunkdscp)
{
	int                round_size; 
	ChunkDescriptor*   chunkdsc;
	void*              chunk;
	int                ret;

	round_size = num_pages(size) * kPageSize;
	
	_lockspace->Lock(0);
	/* 
	 * FIXME: These two allocations have to be atomic. 
	 * We should follow the Ganger rules for allocating storage. 
	 */ 
	if ((ret = _pagepheap->Alloc(round_size, &chunk)) != 0) {
		return -1;
	}
	assert((uintptr_t) chunk % kPageSize == 0);
	if ((ret = _pheap->Alloc(sizeof(*chunkdsc), (void**)&chunkdsc)) != 0) {
		_pagepheap->Free(chunk, round_size);
		_lockspace->Unlock(0);
		return -1;
	}	
	chunkdsc->_owner_id = _principal_id;
	chunkdsc->_chunk = chunk;
	chunkdsc->_size = round_size;
	*chunkdscp = chunkdsc;
	_lockspace->Unlock(0);

	return 0;
}


int 
ChunkStore::DeleteChunk(ChunkDescriptor* chunkdsc)
{
	int   ret;
	int   round_size;
	void* chunk;

	if (!chunkdsc) {
		return -1;
	}

	_lockspace->Lock(0);

	round_size = chunkdsc->_size;
	chunk = chunkdsc->_chunk;

	/* FIXME: access right whether you can free the chunk...
	 * Nevermind, this layer is gonna be implemented in the OS anyways
	 */
	_pheap->Free(chunkdsc, sizeof(*chunkdsc));
	_pagepheap->Free(chunk, round_size);
	_lockspace->Unlock(0);
}


int 
ChunkStore::AccessChunk(ChunkDescriptor* chunkdsc)
{
	int                intret;
	int                r;
	unsigned long long chunkdsc_id = (unsigned long long) chunkdsc;

	intret = client_->call(24, _principal_id, chunkdsc_id, r);

	if (r != 0) {
		return r;
	}

	// TODO
	/* do mprotect */

/*
	std::map<unsigned long long, ChunkMetadata*>::iterator it;
	ChunkMetadata*                                         chunk_metadata;
	unsigned int                                            owner_id;

	it = chunk_metadata_map_.find(chunk_id);
	if (it == chunk_metadata_map_.end()) {
		return -1;
	}	
	chunk_metadata = it->second;
	owner_id = chunk_metadata->get_owner_id();

	return 0;
*/
	return 0;
}
