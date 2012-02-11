#include "chunkstore.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include "rpc/rpc.h"
#include "server/api.h"
#include "common/util.h"
#include "common/vistaheap.h"
#include "chunkdsc.h"
#include "ipc/ipc.h"

static const uint64_t kChunkStoreSize = 1024*1024*64;
static const char*    kChunkStoreName = "chunkstore.vistaheap";


ChunkStore::ChunkStore(::client::Ipc* ipc):
	ipc_(ipc) 
{
}


void 
ChunkStore::Init()
{
	int        ret;
	PHeap*     pheap;

	_pheap = new PHeap();
	_pagepheap = new PHeap();

	while(_pagepheap->Map("chunkstore.page.pheap", kChunkStoreSize, PROT_READ|PROT_WRITE) != 0);
	_pheap->Map("chunkstore.pheap", 1024*1024, PROT_READ| PROT_WRITE);

	_pheap_root = (ChunkStoreRoot*) _pheap->get_root();
}


int
ChunkStore::CreateChunk(size_t size, int type, ChunkDescriptor** chunkdscp)
{
	int                intret; 
	unsigned long long chunkdsc_id;
	unsigned long long r;
	unsigned long long usize = (unsigned long long) size;

	intret = ipc_->call(RPC_CHUNK_CREATE, ipc_->id(), usize, type, r);

	if (r == 0) {
		return -1;
	}

	chunkdsc_id = r;
	*chunkdscp = (ChunkDescriptor*) chunkdsc_id;

	return 0;
}


int 
ChunkStore::DeleteChunk(ChunkDescriptor* chunkdsc)
{
	int                intret; 
	unsigned long long chunkdsc_id = (unsigned long long) chunkdsc;
	int                r;

	if (!chunkdsc) {
		return -1;
	}

	intret = ipc_->call(RPC_CHUNK_DELETE, ipc_->id(), chunkdsc_id, r);

	return 0;
}


int 
ChunkStore::AccessChunkList(ChunkDescriptor* chunkdsc[], size_t nchunkdsc, int prot_flags)
{
	int                             intret;
	int                             r;
	unsigned long long              chunkdsc_id;
	unsigned long long              uaddr;
	std::vector<unsigned long long> vuchunkdsc;
	int                             i;

	for (i=0; i<nchunkdsc; i++) {
		vuchunkdsc.push_back((unsigned long long) chunkdsc[i]);
	}

	intret = ipc_->call(RPC_CHUNK_ACCESS, ipc_->id(), vuchunkdsc, prot_flags, r);

	if (r != 0) {
		return r;
	}

	/* do mprotect */

	for (i=0; i<nchunkdsc; i++) {
		intret = mprotect(chunkdsc[i]->chunk_, chunkdsc[i]->_size, prot_flags);
		assert(intret == 0);
	}

	return 0;
}


int 
ChunkStore::AccessChunk(ChunkDescriptor* chunkdsc, int prot_flags)
{
	ChunkDescriptor* chunkdsc_array[1];

	chunkdsc_array[0] = chunkdsc;

	return AccessChunkList(chunkdsc_array, 1, prot_flags);
}


int 
ChunkStore::ReleaseChunkList(ChunkDescriptor* chunkdsc[], size_t nchunkdsc)
{
	int                             intret;
	int                             r;
	unsigned long long              chunkdsc_id;
	unsigned long long              uaddr;
	std::vector<unsigned long long> vuchunkdsc;
	int                             i;

	for (i=0; i<nchunkdsc; i++) {
		vuchunkdsc.push_back((unsigned long long) chunkdsc[i]);
	}

	intret = ipc_->call(RPC_CHUNK_RELEASE, ipc_->id(), vuchunkdsc, r);

	if (r != 0) {
		return r;
	}

	/* do mprotect */

	for (i=0; i<nchunkdsc; i++) {
		intret = mprotect(chunkdsc[i]->chunk_, chunkdsc[i]->_size, PROT_NONE);
		assert(intret == 0);
	}

	return 0;
}


int 
ChunkStore::ReleaseChunk(ChunkDescriptor* chunkdsc)
{
	ChunkDescriptor* chunkdsc_array[1];

	chunkdsc_array[0] = chunkdsc;

	return ReleaseChunkList(chunkdsc_array, 1);
}



int 
ChunkStore::AccessAddr(void* addr)
{
	int                prot_flags;
	int                intret;
	int                r;
	unsigned long long uaddr;
	std::vector<unsigned long long> vaddr;

	assert(0 && "ChunkStore::AccessAddr not implemented");


	//vaddr.push_back((unsigned long long) addr);

	//intret = _client->call(24, _principal_id, vaddr, r);

	//if (r != 0) {
	//	return r;
	//}


	// TODO
	/* do mprotect */

	return 0;
}
