#include "chunkstore.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rpc/rpc.h"
//#include "common/pheap.h"
#include "common/util.h"
#include "common/vistaheap.h"
#include "chunkdsc.h"


const uint64_t kChunkStoreSize = 1024*1024*64;
const char*    kChunkStoreName = "chunkstore.vistaheap";

ChunkStore::ChunkStore(rpcc* c, id_t principal_id):
	_client(c), 
	_principal_id(principal_id)
{
}


void 
ChunkStore::Init()
{
	int        ret;
	PHeap*     pheap;

	_pheap = new PHeap();
	_pagepheap = new PHeap();

	while(_pagepheap->Map("chunkstore.pagepheap", kChunkStoreSize, 0) != 0);
	_pheap->Map("chunkstore.pheap", 1024*1024, PROT_READ);

}


int
ChunkStore::CreateChunk(size_t size, ChunkDescriptor** chunkdscp)
{
	int                intret; 
	unsigned long long chunkdsc_id;
	unsigned long long r;
	unsigned long long usize = (unsigned long long) size;

	intret = _client->call(22, _principal_id, usize, r);

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

	intret = _client->call(23, _principal_id, chunkdsc_id, r);

	return 0;
}


int 
ChunkStore::AccessChunk(ChunkDescriptor* chunkdsc)
{
	int                prot_flags;
	int                intret;
	int                r;
	unsigned long long chunkdsc_id;
	unsigned long long uaddr;

	chunkdsc_id = (unsigned long long) chunkdsc;
	uaddr = (unsigned long long) chunkdsc->_chunk;

	printf("ChunkStore::AccessChunk: dsc=%p\n", chunkdsc);

	intret = _client->call(24, _principal_id, uaddr, r);

	if (r != 0) {
		return r;
	}

	return 0;

	// TODO
	/* do mprotect */

	prot_flags = PROT_READ;

	printf("ChunkStore::AccessChunk: dsc->chunk=%p\n", chunkdsc->_chunk);
	intret = mprotect(chunkdsc->_chunk, chunkdsc->_size, prot_flags);
	if (intret != 0) {
		return -1;
	}

	return 0;
}


int 
ChunkStore::AccessAddr(void* addr)
{
	int                prot_flags;
	int                intret;
	int                r;
	unsigned long long uaddr;

	uaddr = (unsigned long long) addr;

	printf("ChunkStore::AccessAddr: addr=%p\n", addr);

	intret = _client->call(24, _principal_id, uaddr, r);

	if (r != 0) {
		return r;
	}

	// TODO
	/* do mprotect */

	return 0;
}
