#include "dpo/main/client/salloc.h"
#include <stdlib.h>
#include <typeinfo>
#include <vector>
#include "common/errno.h"
#include "ipc/ipc.h"
#include "dpo/main/common/storage_protocol.h"
#include "client/session.h"



namespace dpo {
namespace client {


// Storage manager
// It allocates storage for a new object:
// 1) from a local pool of objects (that match the ACL), or
// 2) from the kernel storage manager, or
// 3) by contacting the file system server
//
// the allocation function should take a transaction id as argument
// to ensure the atomicity of the operation
//


int
StorageAllocator::AllocateRaw(::client::Session* session, size_t nbytes, void** ptr)
{
	assert(0);
	// FIXME
	/*
	ChunkDescriptor* achunkdsc[16];
	size_t           roundup_bytes = (nbytes % 4096 == 0) ? nbytes: ((nbytes/4096)+1)*4096;

	chunk_store.CreateChunk(roundup_bytes, 0, &achunkdsc[0]);
	*ptr = achunkdsc[0]->chunk_;

	return 0;
	*/
}



// OBSOLETE
int 
StorageAllocator::Alloc(size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
}


// OBSOLETE
int 
StorageAllocator::Alloc(::client::Session* session, size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
	//FIXME
	/*
	ChunkDescriptor* achunkdsc[16];
	size_t           roundup_bytes = (nbytes % 4096 == 0) ? nbytes: ((nbytes/4096)+1)*4096;

	//chunk_store.AccessChunkList(achunkdsc, 1, PROT_READ|PROT_WRITE);
	chunk_store.CreateChunk(roundup_bytes, CHUNK_TYPE_INODE, &achunkdsc[0]);
	*ptr = achunkdsc[0]->chunk_;

	return 0;
	*/
}


int 
StorageAllocator::AllocExtent(::client::Session* session, size_t nbytes, void** ptr)
{
	assert(0);
	//FIXME
	/*
	ChunkDescriptor* achunkdsc[16];
	size_t           roundup_bytes = (nbytes % 4096 == 0) ? nbytes: ((nbytes/4096)+1)*4096;

	chunk_store.CreateChunk(roundup_bytes, CHUNK_TYPE_EXTENT, &achunkdsc[0]);
	*ptr = achunkdsc[0]->chunk_;

	return 0;
	*/
}


int 
StorageAllocator::AllocateContainer(::client::Session* session)
{
	int ret;
	int r;

	int type = 1;
	int num = 2;

	if ((ret = ipc_->call(dpo::StorageProtocol::kAllocateContainer, 
	                      ipc_->id(), type, num, r)) < 0) {
		return ret;
	} else if (ret > 0) {
		return -ret;
	}
	
	return E_SUCCESS;
}


int 
StorageAllocator::AllocateContainerVector(::client::Session* session)
{
	int                                                    ret;
	int                                                    r;
	std::vector< ::dpo::StorageProtocol::ContainerRequest> container_req_vec;
	std::vector<int>                                       rv;
	std::vector<int>::iterator                             rvi;

	::dpo::StorageProtocol::ContainerRequest req;

	req.type = 1;
	req.num = 2;

	container_req_vec.push_back(req);

	ret = ipc_->call(dpo::StorageProtocol::kAllocateContainerVector, 
	                 ipc_->id(), container_req_vec, rv);

	for (rvi = rv.begin(); rvi != rv.end(); rvi++) {
		printf("%d\n", *rvi);
	}
}



} // namespace client
} // namespace dpo
