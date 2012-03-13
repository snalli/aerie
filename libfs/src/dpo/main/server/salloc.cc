#include "dpo/main/server/salloc.h"
#include <stdio.h>
#include <stddef.h>
#include "sal/const.h"
#include "common/errno.h"
#include "common/debug.h"
#include "common/util.h"
#include "ipc/ipc.h"
#include "dpo/main/common/storage_protocol.h"
#include "dpo/containers/set/container.h"
#include "dpo/containers/super/container.h"
#include "dpo/containers/name/container.h"
#include "dpo/containers/byte/container.h"
#include "dpo/main/server/container.h"
#include "ipc/main/server/cltdsc.h"
#include "ipc/main/server/ipc.h"
#include "ipc/main/server/session.h"
#include "dpo/main/server/session.h"


namespace dpo {
namespace server {



StorageAllocator::StorageAllocator(::server::Ipc* ipc)
	: ipc_(ipc)
{ 
	pthread_mutex_init(&mutex_, NULL);
	objtype2factory_map_.set_empty_key(0);
}


int 
StorageAllocator::Make(StoragePool* pool)
{


}


int 
StorageAllocator::Load(StoragePool* pool) 
{
	void*                                                                 b;
	dpo::containers::server::SuperContainer::Object*                      super_obj;
	dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object* set_obj;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(server_salloc), 
	        "Load storage pool 0x%lx\n", pool->Identity());

	if ((b = pool->root()) == 0) {
		return -E_NOENT;
	}
	if ((super_obj = dpo::containers::server::SuperContainer::Object::Load(b)) == NULL) {
		return -E_NOMEM;
	}
	dpo::common::ObjectId freelist_oid = super_obj->freelist(NULL); // we don't need journaling
	if ((set_obj = dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object::Load(freelist_oid)) == NULL) {
		return -E_NOMEM;
	}
	
	pool_ = pool;
	freeset_ = set_obj;
	printf("%p, LOAD pool_=%p\n", this, pool_);
	
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


#endif

/*
int
StorageAllocator::AllocateExtent(DpoSession* session, size_t nbytes, void** ptr)
{


}
*/


int
StorageAllocator::RegisterBaseTypes()
{
	int ret;

	{
	// NameContainer
	::dpo::server::ContainerAbstractFactory* factory = new dpo::containers::server::NameContainer::Factory();
    if ((ret = RegisterType(dpo::containers::T_NAME_CONTAINER, factory)) < 0) {
		return ret;
	}
	}

	{
	// ByteContainer
	::dpo::server::ContainerAbstractFactory* factory = new dpo::containers::server::ByteContainer::Factory();
    if ((ret = RegisterType(dpo::containers::T_BYTE_CONTAINER, factory)) < 0) {
		return ret;
	}
	}

	return E_SUCCESS;
}


int
StorageAllocator::RegisterType(::dpo::common::ObjectType type_id, 
                               ::dpo::server::ContainerAbstractFactory* objfactory)
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
StorageAllocator::Alloc(DpoSession* session, size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
}


int 
StorageAllocator::AllocateExtent(DpoSession* session, size_t size, void** ptr)
{
	int ret;

	if ((ret = pool_->AllocateExtent(size, ptr)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


int 
StorageAllocator::AllocateContainer(DpoSession* session, int type, int num, ::dpo::StorageProtocol::ContainerReply& reply)
{
	dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object* set_obj;
	dpo::common::ObjectId                                                 oid;
	dpo::common::ObjectId                                                 set_oid;
	int                                                                   ret;
	char*                                                                 buffer;
	size_t                                                                static_size;
	size_t                                                                extent_size;
	bool                                                                  found = false;
	::dpo::common::Object*                                                obj;

	DBG_LOG(DBG_INFO, DBG_MODULE(server_salloc), 
	        "[%d] Allocate container %d\n", session->clt(), type);

	for (int i=0; i<freeset_->Size(); i++) {
		freeset_->Read(session, i, &set_oid);
		printf("set_oid=%lu\n", set_oid.u64());
		if ((set_obj = dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object::Load(set_oid)) == NULL) {
			break;
		}
		if (set_obj->Size() > 0 && set_obj->Read(session, 0, &oid) == SUCCESS) {
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
	extent_size = sizeof(dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object);
	if ((ret = pool_->AllocateExtent(extent_size, (void**) &buffer)) < 0) {
		return ret;
	}
	if ((set_obj = dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object::Make(session, buffer)) == NULL) {
		return -E_NOMEM;
	}

	// Create objects and insert them into the set 
	static_size = objtype2factory_map_[type]->StaticSize();
	if (static_size < kBlockSize) {
		static_size = RoundUpSize(static_size, ::dpo::common::kObjectAlignSize);
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
    salloc_->ipc_->reg(::dpo::StorageProtocol::kAllocateContainer, this, 
	                   &::dpo::server::StorageAllocator::IpcHandlers::AllocateContainer);
    salloc_->ipc_->reg(::dpo::StorageProtocol::kAllocateContainerVector, this, 
	                   &::dpo::server::StorageAllocator::IpcHandlers::AllocateContainerVector);

	return E_SUCCESS;
}


int 
StorageAllocator::IpcHandlers::AllocateContainer(int clt, int type, int num, 
                                                 ::dpo::StorageProtocol::ContainerReply& r)
{
	int                    ret;
	::server::BaseSession* session;
	
	if ((ret = salloc_->ipc_->session_manager()->Lookup(clt, &session)) < 0) {
		return -ret;
	}
	if ((ret = salloc_->AllocateContainer(static_cast<DpoSession*>(session), 
	                                      type, num, r)) < 0) {
		return -ret;
	}
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
