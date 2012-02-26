#include "dpo/main/server/salloc.h"
#include <stdio.h>
#include <stddef.h>
#include "common/errno.h"
#include "ipc/ipc.h"
#include "dpo/main/common/storage_protocol.h"
#include "dpo/containers/set/container.h"
#include "ipc/main/server/cltdsc.h"
#include "server/session.h"


namespace dpo {
namespace server {


//
// Group Containers per (type, ACL) 
//
//

class ContainersPerClient {
public:
	// verify_allocation (capability)
	// 
	

private:
	// index of available containers to client 


};


StorageAllocator::StorageAllocator(::server::Ipc* ipc, dpo::server::Dpo* dpo)
	: ipc_(ipc),
	  dpo_(dpo),
	  descriptor_count_(0)
{ }


int
StorageAllocator::Init()
{
	int                   ret;

	if (ipc_) {
		return ipc_handlers_.Register(this);
	}
	return E_SUCCESS;
}


/*
int
StorageAllocator::RegisterStorageContainer(dpo::common::ObjectId oid)
{
	stcnr_oid_ = oid;
	return E_SUCCESS;
}
*/


#if 0

// Makes the OS dependent call to allocate space
int
StorageAllocator::CreateExtent(int acl)
{
	

}


int
StorageAllocator::CreateStorageContainer()
{


}


int 
StorageAllocator::AllocateContainerInternal(int clt, int type, int acl)
{


}



int 
StorageAllocator::AllocateContainer(int clt, int type, int acl, int n)
{


}

#endif


int
StorageAllocator::AllocateRaw(::server::Session* session, size_t nbytes, void** ptr)
{
	assert(0);
	/*
	ChunkDescriptor* achunkdsc[16];
	size_t           roundup_bytes = (nbytes % 4096 == 0) ? nbytes: ((nbytes/4096)+1)*4096;

	chunk_server->CreateChunk(0, roundup_bytes, 0, &achunkdsc[0]);
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
StorageAllocator::Alloc(::server::Session* session, size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
	/*
	ChunkDescriptor* achunkdsc[16];
	size_t           roundup_bytes = (nbytes % 4096 == 0) ? nbytes: ((nbytes/4096)+1)*4096;

	//chunk_store.AccessChunkList(achunkdsc, 1, PROT_READ|PROT_WRITE);
	chunk_server->CreateChunk(0, roundup_bytes, CHUNK_TYPE_INODE, &achunkdsc[0]);
	*ptr = achunkdsc[0]->chunk_;

	return 0;
	*/
}


int 
StorageAllocator::AllocateContainer(::server::Session* session, int type, int num)
{
	/*
	dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object* stcnr = 
		dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object::Load(stcnr_oid_); 
	
	printf("ALLOCATE CONTAINER: %p\n", stcnr);

	dpo::common::ObjectId oid0 = dpo::common::ObjectId(10);
	dpo::common::ObjectId oid1 = dpo::common::ObjectId(11);
	dpo::common::ObjectId oid2 = dpo::common::ObjectId(12);
	dpo::common::ObjectId oid;
	
	stcnr->Insert(session, oid0);
	stcnr->Insert(session, oid1);
	stcnr->Insert(session, oid2);
	stcnr->Read(session, 1, &oid);
	printf("oid=%lu\n", oid.u64());
	*/
}




int
StorageAllocator::IpcHandlers::Register(StorageAllocator* salloc)
{
	salloc_ = salloc;
    salloc_->ipc_->reg(::dpo::StorageProtocol::kAllocateContainer, this, 
	                 &::dpo::server::StorageAllocator::IpcHandlers::AllocateContainer);
    salloc_->ipc_->reg(::dpo::StorageProtocol::kAllocateContainerVector, this, 
	                 &::dpo::server::StorageAllocator::IpcHandlers::AllocateContainerVector);

	return E_SUCCESS;
}


//FIXME: what do we return? a capability, a hint, oid?
//StorageAllocator::AllocateContainer(int clt, int type, int num, ::dpo::StorageProtocol::Capability& cap)
int 
StorageAllocator::IpcHandlers::AllocateContainer(int clt, int type, int num, int& r)
{
	int ret;

	//::server::Session session(salloc_->dpo_);

	//FIXME: we should get session for the client descriptor
	//if ((ret = salloc_->AllocateContainer(&session, type, num)) < 0) {
	//	return -ret;
	//}
	return E_SUCCESS;
}


int 
StorageAllocator::IpcHandlers::AllocateContainerVector(int clt,
                                                     std::vector< ::dpo::StorageProtocol::ContainerRequest> container_req_vec, 
                                                     std::vector<int>& result)
{
	std::vector< ::dpo::StorageProtocol::ContainerRequest>::iterator vit;
	
	for (vit = container_req_vec.begin(); vit != container_req_vec.end(); vit++) {
		//printf("type: %d, num=%d\n", (*vit).type, (*vit).num);
		
		result.push_back(1);
	}

	return E_SUCCESS;
}


} // namespace server
} // namespace dpo
