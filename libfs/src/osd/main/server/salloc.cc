#include "osd/main/server/salloc.h"
#include <stdio.h>
#include <stddef.h>
#include "spa/const.h"
#include "common/errno.h"
#include "common/util.h"
#include "bcs/bcs.h"
#include "osd/main/common/storage_protocol.h"
#include "osd/containers/set/container.h"
#include "osd/containers/super/container.h"
#include "osd/containers/name/container.h"
#include "osd/containers/byte/container.h"
#include "osd/main/server/container.h"
#include "osd/main/server/session.h"


namespace osd {
namespace server {


StorageAllocator::StorageAllocator(::server::Ipc* ipc, StoragePool* pool, FreeSet* freeset)
	: ipc_(ipc),
	  pool_(pool),
	  freeset_(freeset)
{ 
	pthread_mutex_init(&mutex_, NULL);
	objtype2factory_map_.set_empty_key(0);
}


int 
StorageAllocator::Load(::server::Ipc* ipc, StoragePool* pool, StorageAllocator** sallocp) 
{
	void*                                               b;
	osd::containers::server::SuperContainer::Object*    super_obj;
	FreeSet*                                            set_obj;
	StorageAllocator*                                   salloc;
	int                                                 ret;

	DBG_LOG(DBG_INFO, DBG_MODULE(server_salloc), 
	        "Load storage pool 0x%lx\n", pool->Identity());

	if ((b = pool->root()) == 0) {
		return -E_NOENT;
	}
	if ((super_obj = osd::containers::server::SuperContainer::Object::Load(b)) == NULL) {
		return -E_NOMEM;
	}
	osd::common::ObjectId freelist_oid = super_obj->freelist(NULL); // we don't need journaling
	if ((set_obj = FreeSet::Object::Load(freelist_oid)) == NULL) {
		return -E_NOMEM;
	}
	if ((salloc = new StorageAllocator(ipc, pool, set_obj)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = salloc->Init()) < 0) {
		return ret;
	}
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
	if ((ret = LoadFreeMap()) < 0) {
		return ret;
	}
	if (ipc_) {
		return ipc_handlers_.Register(this);
	}
	return E_SUCCESS;
}


int
StorageAllocator::LoadFreeMap()
{
	AclObjectPair aclobjpair;
	ObjectIdSet*  oid_set;
	
	for (int i=0; i<freeset_->Size(); i++) {
		freeset_->Read(NULL, i, &aclobjpair);
		if ((oid_set = ObjectIdSet::Load(aclobjpair.oid)) == NULL) {
			break;
		}
		freemap_.insert(std::pair<osd::common::AclIdentifier, ObjectIdSet*>(aclobjpair.acl_id, oid_set));
	}

	return E_SUCCESS;
}


int
StorageAllocator::RegisterBaseTypes()
{
	int ret;

	{
	// NameContainer
	::osd::server::ContainerAbstractFactory* factory = new osd::containers::server::NameContainer::Factory();
    if ((ret = RegisterType(osd::containers::T_NAME_CONTAINER, factory)) < 0) {
		return ret;
	}
	}

	{
	// ByteContainer
	::osd::server::ContainerAbstractFactory* factory = new osd::containers::server::ByteContainer::Factory();
    if ((ret = RegisterType(osd::containers::T_BYTE_CONTAINER, factory)) < 0) {
		return ret;
	}
	}

	return E_SUCCESS;
}


int
StorageAllocator::RegisterType(::osd::common::ObjectType type_id, 
                               ::osd::server::ContainerAbstractFactory* objfactory)
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
StorageAllocator::Alloc(OsdSession* session, size_t nbytes, std::type_info const& typid, void** ptr)
{
	assert(0);
}


int 
StorageAllocator::AllocateExtent(OsdSession* session, size_t size, int flags, void** ptr)
{
	int ret;

	if ((ret = pool_->AllocateExtent(size, ptr)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


int 
StorageAllocator::AllocateExtent(OsdSession* session, ObjectIdSet* set, int size, 
                                 int count, int& reply)
{
	int                     ret;
	size_t                  extent_size;
	char*                   buffer;
	::osd::common::ExtentId eid;

	DBG_LOG(DBG_INFO, DBG_MODULE(server_salloc), 
	        "[%d] Allocate %d extents(s) of size %d\n", session->clt(), count, size);

	extent_size = count * size;
	if ((ret = pool_->AllocateExtent(extent_size, (void**) &buffer)) < 0) {
		return ret;
	}

	for (size_t s=0; s<extent_size; s+=size) {
		char* b = (char*) buffer + s;
		eid = ::osd::common::ExtentId(b, size);
		set->Insert(session, eid);
	}
	
	return E_SUCCESS;
}


// not thread safe
int
StorageAllocator::CreateContainerSet(OsdSession* session, osd::common::AclIdentifier acl_id, 
                                     ObjectIdSet::Object** obj_set)
{
	char*   buffer;
	size_t  extent_size;
	int     ret;

	extent_size = sizeof(ObjectIdSet::Object);
	if ((ret = pool_->AllocateExtent(extent_size, (void**) &buffer)) < 0) {
		return ret;
	}
	if ((*obj_set = ObjectIdSet::Object::Make(session, buffer)) == NULL) {
		return -E_NOMEM;
	}

	return E_SUCCESS;
}


// allocates a set of containers (actually a set of object ids of containers)
// set may already contain containers if instantiated from an existing set
int
StorageAllocator::AllocateObjectIdSet(OsdSession* session, osd::common::AclIdentifier  acl_id, 
                                      ::osd::StorageProtocol::ContainerReply& reply)
{
	int                   ret;
	FreeMap::iterator     it;
	ObjectIdSet*          obj_set;
	osd::common::ObjectId oid;

	DBG_LOG(DBG_INFO, DBG_MODULE(server_salloc), 
	        "[%d] Allocate ObjectID Set of ACL %d\n", session->clt(), acl_id);

	pthread_mutex_lock(&mutex_);

	// find an existing set matching the acl or create a new one
	it = freemap_.find(acl_id);
	if (it == freemap_.end()) {
		if ((ret = CreateContainerSet(session, acl_id, &obj_set)) < 0) {
			goto done;
		}
	} else {
		oid = it->second->oid();
		if ((obj_set = osd::containers::server::SetContainer<osd::common::ObjectId>::Object::Load(oid)) == NULL) {
			ret = -E_NOMEM;
			goto done;
		}
		freemap_.erase(it);
	}
	// Assign the set to the client
	reply.oid = obj_set->oid();
	reply.index = session->sets_.size(); // location to index the table of assigned free sets
	session->sets_.push_back(reply.oid);
	ret = E_SUCCESS;
done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


// Allocate containers and put then in the set
int 
StorageAllocator::AllocateContainer(OsdSession* session, ObjectIdSet* set, int type, 
                                    int count, int& reply)
{
	int                     ret;
	char*                   buffer;
	size_t                  static_size;
	size_t                  extent_size;
	::osd::common::Object*  obj;

	DBG_LOG(DBG_INFO, DBG_MODULE(server_salloc), 
	        "[%d] Allocate %d container(s) of type %d\n", session->clt(), count, type);

	// Create objects and insert them into the set 
	static_size = objtype2factory_map_[type]->StaticSize();
	if (static_size < kBlockSize) {
		static_size = RoundUpSize(static_size, ::osd::common::kObjectAlignSize);
	} else {
		static_size = NumOfBlocks(static_size, kBlockSize) * kBlockSize;
	}
	extent_size = count * static_size;
	if ((ret = pool_->AllocateExtent(extent_size, (void**) &buffer)) < 0) {
		return ret;
	}
	for (size_t s=0; s<extent_size; s+=static_size) {
		char* b = (char*) buffer + s;
		if ((obj = objtype2factory_map_[type]->Make(session, b)) == NULL) {
			return -E_NOMEM;
		}
		set->Insert(session, obj->oid());
	}
	
	return E_SUCCESS;
}


int
StorageAllocator::IpcHandlers::Register(StorageAllocator* salloc)
{
	salloc_ = salloc;
    salloc_->ipc_->reg(::osd::StorageProtocol::kAllocateObjectIdSet, this, 
	                   &::osd::server::StorageAllocator::IpcHandlers::AllocateObjectIdSet);
    salloc_->ipc_->reg(::osd::StorageProtocol::kAllocateExtent, this, 
	                   &::osd::server::StorageAllocator::IpcHandlers::AllocateExtent);
    salloc_->ipc_->reg(::osd::StorageProtocol::kAllocateContainer, this, 
	                   &::osd::server::StorageAllocator::IpcHandlers::AllocateContainer);
    salloc_->ipc_->reg(::osd::StorageProtocol::kAllocateContainerVector, this, 
	                   &::osd::server::StorageAllocator::IpcHandlers::AllocateContainerVector);

	return E_SUCCESS;
}


int 
StorageAllocator::IpcHandlers::AllocateObjectIdSet(int clt, osd::common::AclIdentifier acl_id, 
                                                   ::osd::StorageProtocol::ContainerReply& r)
{
	int                    ret;
	::server::BaseSession* session;
	
	if ((ret = salloc_->ipc_->session_manager()->Lookup(clt, &session)) < 0) {
		return -ret;
	}
	if ((ret = salloc_->AllocateObjectIdSet(static_cast<OsdSession*>(session), 
	                                         acl_id, r)) < 0) {
		return -ret;
	}
	return E_SUCCESS;
}


// set_capability is an index into the table of container sets allocates to the client
// new containers will be allocated into the set pointed by the set_capability
int 
StorageAllocator::IpcHandlers::AllocateContainer(int clt, int set_capability, int type, int num, 
                                                 int& reply)
{
	int                    ret;
	::server::BaseSession* session;
	OsdSession*            osdsession;
	ObjectIdSet*           obj_set;
	osd::common::ObjectId  oid;
	
	if ((ret = salloc_->ipc_->session_manager()->Lookup(clt, &session)) < 0) {
		return -ret;
	}
	osdsession = static_cast<OsdSession*>(session);
	assert(set_capability < osdsession->sets_.size());
	oid = osdsession->sets_[set_capability];

	if ((obj_set = osd::containers::server::SetContainer<osd::common::ObjectId>::Object::Load(oid)) == NULL) {
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(server_salloc), "[%d] Cannot load persistent object\n", clt);
	}
	if ((ret = salloc_->AllocateContainer(osdsession, obj_set, type, num, reply)) < 0) {
		return -ret;
	}
	return E_SUCCESS;
}


// set_capability is an index into the table of container sets allocates to the client
// new containers will be allocated into the set pointed by the set_capability
int 
StorageAllocator::IpcHandlers::AllocateExtent(int clt, int set_capability, int size, int num, 
                                              int& reply)
{
	int                    ret;
	::server::BaseSession* session;
	OsdSession*            osdsession;
	ObjectIdSet*           obj_set;
	osd::common::ObjectId  oid;
	
	if ((ret = salloc_->ipc_->session_manager()->Lookup(clt, &session)) < 0) {
		return -ret;
	}
	osdsession = static_cast<OsdSession*>(session);
	assert(set_capability < osdsession->sets_.size());
	oid = osdsession->sets_[set_capability];

	if ((obj_set = osd::containers::server::SetContainer<osd::common::ObjectId>::Object::Load(oid)) == NULL) {
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(server_salloc), "[%d] Cannot load persistent object\n", clt);
	}
	if ((ret = salloc_->AllocateExtent(osdsession, obj_set, size, num, reply)) < 0) {
		return -ret;
	}
	return E_SUCCESS;
}



int 
StorageAllocator::IpcHandlers::AllocateContainerVector(int clt,
                                                       std::vector< ::osd::StorageProtocol::ContainerRequest> container_req_vec, 
                                                       std::vector<int>& result)
{
	std::vector< ::osd::StorageProtocol::ContainerRequest>::iterator vit;
	
	for (vit = container_req_vec.begin(); vit != container_req_vec.end(); vit++) {
		//printf("type: %d, num=%d\n", (*vit).type, (*vit).num);
		
		result.push_back(1);
	}

	return E_SUCCESS;
}


} // namespace server
} // namespace osd
