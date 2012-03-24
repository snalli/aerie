#include "ssa/main/server/salloc.h"
#include <stdio.h>
#include <stddef.h>
#include "spa/const.h"
#include "common/errno.h"
#include "common/util.h"
#include "bcs/bcs.h"
#include "ssa/main/common/storage_protocol.h"
#include "ssa/containers/set/container.h"
#include "ssa/containers/super/container.h"
#include "ssa/containers/name/container.h"
#include "ssa/containers/byte/container.h"
#include "ssa/main/server/container.h"
#include "ssa/main/server/session.h"


namespace ssa {
namespace server {



StorageAllocator::StorageAllocator(::server::Ipc* ipc, StoragePool* pool)
	: ipc_(ipc),
	  pool_(pool)
{ 
	pthread_mutex_init(&mutex_, NULL);
	objtype2factory_map_.set_empty_key(0);
}


int 
StorageAllocator::Load(::server::Ipc* ipc, StoragePool* pool, StorageAllocator** sallocp) 
{
	void*                                                                 b;
	ssa::containers::server::SuperContainer::Object*                      super_obj;
	ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object* set_obj;
	StorageAllocator*                                                     salloc;
	int                                                                   ret;

	DBG_LOG(DBG_INFO, DBG_MODULE(server_salloc), 
	        "Load storage pool 0x%lx\n", pool->Identity());

	if ((b = pool->root()) == 0) {
		return -E_NOENT;
	}
	if ((super_obj = ssa::containers::server::SuperContainer::Object::Load(b)) == NULL) {
		return -E_NOMEM;
	}
	ssa::common::ObjectId freelist_oid = super_obj->freelist(NULL); // we don't need journaling
	if ((set_obj = ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object::Load(freelist_oid)) == NULL) {
		return -E_NOMEM;
	}
	if ((salloc = new StorageAllocator(ipc, pool)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = salloc->Init()) < 0) {
		return ret;
	}
	salloc->freeset_ = set_obj;
	salloc->can_commit_suicide_ = true;
	*sallocp = salloc;

	return E_SUCCESS;
}


int 
StorageAllocator::Close()
{
	if (can_commit_suicide_) {
		delete this;
	}
	return E_SUCCESS;
}


int
StorageAllocator::Init()
{
	int ret;

	if ((ret = RegisterBaseTypes()) < 0) {
		return ret;
	}
	if (ipc_) {
		return ipc_handlers_.Register(this);
	}
	return E_SUCCESS;
}


/*
int
StorageAllocator::RegisterStorageContainer(ssa::common::ObjectId oid)
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


#endif

/*
int
StorageAllocator::AllocateExtent(SsaSession* session, size_t nbytes, void** ptr)
{


}
*/


int
StorageAllocator::RegisterBaseTypes()
{
	int ret;

	{
	// NameContainer
	::ssa::server::ContainerAbstractFactory* factory = new ssa::containers::server::NameContainer::Factory();
    if ((ret = RegisterType(ssa::containers::T_NAME_CONTAINER, factory)) < 0) {
		return ret;
	}
	}

	{
	// ByteContainer
	::ssa::server::ContainerAbstractFactory* factory = new ssa::containers::server::ByteContainer::Factory();
    if ((ret = RegisterType(ssa::containers::T_BYTE_CONTAINER, factory)) < 0) {
		return ret;
	}
	}

	return E_SUCCESS;
}


int
StorageAllocator::RegisterType(::ssa::common::ObjectType type_id, 
                               ::ssa::server::ContainerAbstractFactory* objfactory)
{
	int                          ret = E_SUCCESS;
	ObjectType2Factory::iterator itr; 

	pthread_mutex_lock(&mutex_);
    if ((itr = objtype2factory_map_.find(type_id)) != objtype2factory_map_.end()) {
		ret = -E_EXIST;
		goto done;
	}
	objtype2factory_map_[type_id] = objfactory;

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


// OBSOLETE
int 
StorageAllocator::Alloc(size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
}
 

// OBSOLETE
int 
StorageAllocator::Alloc(SsaSession* session, size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
}


int 
StorageAllocator::AllocateExtent(SsaSession* session, size_t size, int flags, void** ptr)
{
	int ret;

	if ((ret = pool_->AllocateExtent(size, ptr)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}



int 
StorageAllocator::AllocateContainer(SsaSession* session, int type, int num, ::ssa::StorageProtocol::ContainerReply& reply)
{
	ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object* set_obj;
	ssa::common::ObjectId                                                 oid;
	ssa::common::ObjectId                                                 set_oid;
	int                                                                   ret;
	char*                                                                 buffer;
	size_t                                                                static_size;
	size_t                                                                extent_size;
	bool                                                                  found = false;
	::ssa::common::Object*                                                obj;

	DBG_LOG(DBG_INFO, DBG_MODULE(server_salloc), 
	        "[%d] Allocate container %d\n", session->clt(), type);
	
	for (int i=0; i<freeset_->Size(); i++) {
		freeset_->Read(session, i, &set_oid);
		printf("set_oid=%lu\n", set_oid.u64());
		if ((set_obj = ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object::Load(set_oid)) == NULL) {
			break;
		}
		if (set_obj->Size() > 0 && set_obj->Read(session, 0, &oid) == E_SUCCESS) {
			if (oid.type() == type) {
				found = true;
				break;
			}
		}
	}

	if (found) {
		printf("ALLOCATE EXISTING\n");
		return E_SUCCESS;
	}

	// Create a new set
	extent_size = sizeof(ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object);
	if ((ret = pool_->AllocateExtent(extent_size, (void**) &buffer)) < 0) {
		return ret;
	}
	if ((set_obj = ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object::Make(session, buffer)) == NULL) {
		return -E_NOMEM;
	}

	// Create objects and insert them into the set 
	static_size = objtype2factory_map_[type]->StaticSize();
	if (static_size < kBlockSize) {
		static_size = RoundUpSize(static_size, ::ssa::common::kObjectAlignSize);
	} else {
		static_size = NumOfBlocks(static_size, kBlockSize) * kBlockSize;
	}
	extent_size = num * static_size;
	if ((ret = pool_->AllocateExtent(extent_size, (void**) &buffer)) < 0) {
		return ret;
	}
	for (size_t s=0; s<extent_size; s+=static_size) {
		char* b = (char*) buffer + s;
		if ((obj = objtype2factory_map_[type]->Make(session, b)) == NULL) {
			return -E_NOMEM;
		}
		set_obj->Insert(session, obj->oid());
	}
	// Assign the set to the client
	reply.oid = set_obj->oid();
	reply.index = session->sets_.size();
	session->sets_.push_back(reply.oid);
	return E_SUCCESS;
}


int
StorageAllocator::IpcHandlers::Register(StorageAllocator* salloc)
{
	salloc_ = salloc;
    salloc_->ipc_->reg(::ssa::StorageProtocol::kAllocateContainer, this, 
	                   &::ssa::server::StorageAllocator::IpcHandlers::AllocateContainer);
    salloc_->ipc_->reg(::ssa::StorageProtocol::kAllocateContainerVector, this, 
	                   &::ssa::server::StorageAllocator::IpcHandlers::AllocateContainerVector);

	return E_SUCCESS;
}


int 
StorageAllocator::IpcHandlers::AllocateContainer(int clt, int type, int num, 
                                                 ::ssa::StorageProtocol::ContainerReply& r)
{
	int                    ret;
	::server::BaseSession* session;
	
	if ((ret = salloc_->ipc_->session_manager()->Lookup(clt, &session)) < 0) {
		return -ret;
	}
	if ((ret = salloc_->AllocateContainer(static_cast<SsaSession*>(session), 
	                                      type, num, r)) < 0) {
		return -ret;
	}
	return E_SUCCESS;
}


int 
StorageAllocator::IpcHandlers::AllocateContainerVector(int clt,
                                                       std::vector< ::ssa::StorageProtocol::ContainerRequest> container_req_vec, 
                                                       std::vector<int>& result)
{
	std::vector< ::ssa::StorageProtocol::ContainerRequest>::iterator vit;
	
	for (vit = container_req_vec.begin(); vit != container_req_vec.end(); vit++) {
		//printf("type: %d, num=%d\n", (*vit).type, (*vit).num);
		
		result.push_back(1);
	}

	return E_SUCCESS;
}


} // namespace server
} // namespace ssa
