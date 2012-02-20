#include "dpo/main/server/smgr.h"
#include <stdio.h>
#include <stddef.h>
#include "common/errno.h"
#include "ipc/ipc.h"
#include "dpo/main/common/storage_protocol.h"
#include "dpo/containers/set/container.h"
#include "ipc/main/server/cltdsc.h"
#include "server/session.h"
#include "chunkstore/chunkserver.h"

//TO DEPRECATE
extern ::ChunkServer* chunk_server;


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


StorageManager::StorageManager(::server::Ipc* ipc, dpo::server::Dpo* dpo)
	: ipc_(ipc),
	  dpo_(dpo),
	  descriptor_count_(0)
{ }


int
StorageManager::Init()
{
	int                   ret;

	StorageClientDescriptor::Register();

	return ipc_handlers_.Register(this);
}


int
StorageManager::RegisterPartition(const char* dev, dpo::common::ObjectId stcnr)
{
	//TODO: this registers the partition we allocate raw blocks from
	return E_SUCCESS;
}


/*
int
StorageManager::RegisterStorageContainer(dpo::common::ObjectId oid)
{
	stcnr_oid_ = oid;
	return E_SUCCESS;
}
*/


#if 0

// Makes the OS dependent call to allocate space
int
StorageManager::CreateExtent(int acl)
{
	

}


int
StorageManager::CreateStorageContainer()
{


}


int 
StorageManager::AllocateContainerInternal(int clt, int type, int acl)
{


}



int 
StorageManager::AllocateContainer(int clt, int type, int acl, int n)
{


}

#endif


int
StorageManager::AllocateRaw(::server::Session* session, size_t nbytes, void** ptr)
{
	ChunkDescriptor* achunkdsc[16];
	size_t           roundup_bytes = (nbytes % 4096 == 0) ? nbytes: ((nbytes/4096)+1)*4096;

	chunk_server->CreateChunk(0, roundup_bytes, 0, &achunkdsc[0]);
	*ptr = achunkdsc[0]->chunk_;

	return 0;
}


// OBSOLETE
int 
StorageManager::Alloc(size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
}


// OBSOLETE
int 
StorageManager::Alloc(::server::Session* session, size_t nbytes, std::type_info const& typid, void** ptr)
{
	ChunkDescriptor* achunkdsc[16];
	size_t           roundup_bytes = (nbytes % 4096 == 0) ? nbytes: ((nbytes/4096)+1)*4096;

	//chunk_store.AccessChunkList(achunkdsc, 1, PROT_READ|PROT_WRITE);
	chunk_server->CreateChunk(0, roundup_bytes, CHUNK_TYPE_INODE, &achunkdsc[0]);
	*ptr = achunkdsc[0]->chunk_;

	return 0;
}


int 
StorageManager::AllocateContainer(::server::Session* session, int type, int num)
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
StorageManager::IpcHandlers::Register(StorageManager* smgr)
{
	smgr_ = smgr;
    smgr_->ipc_->reg(::dpo::StorageProtocol::kAllocateContainer, this, 
	                 &::dpo::server::StorageManager::IpcHandlers::AllocateContainer);
    smgr_->ipc_->reg(::dpo::StorageProtocol::kAllocateContainerVector, this, 
	                 &::dpo::server::StorageManager::IpcHandlers::AllocateContainerVector);


	return E_SUCCESS;
}


//FIXME: what do we return? a capability, a hint, oid?
//StorageManager::AllocateContainer(int clt, int type, int num, ::dpo::StorageProtocol::Capability& cap)
int 
StorageManager::IpcHandlers::AllocateContainer(int clt, int type, int num, int& r)
{
	int ret;

	::server::Session session(smgr_->dpo_);

	//FIXME: we should get session for the client descriptor
	if ((ret = smgr_->AllocateContainer(&session, type, num)) < 0) {
		return -ret;
	}
	return E_SUCCESS;
}


int 
StorageManager::IpcHandlers::AllocateContainerVector(int clt, 
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
