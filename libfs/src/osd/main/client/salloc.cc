#include "osd/main/client/salloc.h"
#include <stdlib.h>
#include <typeinfo>
#include <vector>
#include "common/errno.h"
#include "bcs/bcs.h"
#include "osd/main/common/storage_protocol.h"
#include "osd/main/client/session.h"
#include "osd/main/common/publisher.h"
#include "osd/containers/set/container.h"



namespace osd {
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
ContainerPool::LoadFromLast(OsdSession* session)
{
	osd::common::ObjectId                                                 oid;
	osd::common::Object*                                                  obj;

	for (int i = last_offset_; i < set_obj_->Size(); i++) {
		set_obj_->Read(session, i, &oid);
		obj = osd::common::Object::Load(oid);
		freelist_[obj->type()].push_back(FreeContainerDescriptor(oid, obj->type()));
	}
	last_offset_ = set_obj_->Size();
	return E_SUCCESS;
}


int
ContainerPool::Create(::client::Ipc* ipc, OsdSession* session, 
                      osd::common::AclIdentifier acl_id, 
                      ContainerPool** poolp)
{
	::osd::StorageProtocol::ContainerReply  reply;
	ContainerPool*                          pool;
	int                                     ret;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_salloc), 
	        "[%d] Allocate a pool of containers with ACL %d\n", ipc->id(), acl_id);
	
	if ((ret = ipc->call(osd::StorageProtocol::kAllocateContainerSet, 
	                     ipc->id(), acl_id, reply)) < 0) {
		return ret;
	} else if (ret > 0) {
		return -ret;
	}
	if ((pool = new ContainerPool(reply.index, reply.oid)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = pool->LoadFromLast(session)) < 0) {
		return ret;
	}
	*poolp = pool;
		
	return E_SUCCESS;
}


int 
ContainerPool::Allocate(::client::Ipc* ipc, OsdSession* session, int type, osd::common::ObjectId* oid)
{
	int r;
	int ret;


	if (freelist_[type].empty()) {
		if ((ret = ipc->call(osd::StorageProtocol::kAllocateContainer, 
							 ipc->id(), capability_, type, 256, r)) < 0) {
			return ret;
		} else if (ret > 0) {
			return -ret;
		}
		if ((ret = LoadFromLast(session)) < 0) {
			return ret;
		}
	}
	FreeContainerDescriptor& front = freelist_[type].front();
	*oid = front.oid_;
	//TODO: mark the allocation down in the journal.
	freelist_[type].pop_front();
	return E_SUCCESS;
}


int 
StorageAllocator::Load(StoragePool* pool) 
{
	pool_ = pool;
	return E_SUCCESS;
}


int
StorageAllocator::AllocateExtent(OsdSession* session, size_t nbytes, int flags, void** ptr)
{
	if (flags & kMetadata) {
		*ptr = malloc(nbytes);
	} else {
		session->journal() << osd::Publisher::Message::ContainerOperation::AllocateExtent();
		return pool_->AllocateExtent(nbytes, ptr);
	}
	return E_SUCCESS;
}


int 
StorageAllocator::GetContainerPool(OsdSession* session, osd::common::AclIdentifier acl_id, 
                                   ContainerPool** poolp)
{
	int                  ret;
	AclPoolMap::iterator it;
	ContainerPool*       pool;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_salloc), 
	        "[%d] Allocate container set of ACL %d\n", ipc_->id(), acl_id);

	if ((it = aclpoolmap_.find(acl_id)) == aclpoolmap_.end()) {
		if ((ret = ContainerPool::Create(ipc_, session, acl_id, &pool)) < 0) {
			return ret;
		}
		aclpoolmap_.insert(std::pair<osd::common::AclIdentifier, ContainerPool*>(acl_id, pool));
	} else {
		pool = it->second;	
	}
	*poolp = pool;
	
	return E_SUCCESS;
}


int 
StorageAllocator::AllocateContainer(OsdSession* session, osd::common::AclIdentifier acl_id, 
                                    int type, osd::common::ObjectId* oidp)
{
	int                   ret;
	ContainerPool*        pool;
	osd::common::ObjectId oid;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_salloc), 
	        "[%d] Allocate container of type %d\n", ipc_->id(), type);

	if ((ret = GetContainerPool(session, acl_id, &pool)) < 0) {
		return ret;
	}
	if ((ret = pool->Allocate(ipc_, session, type, &oid)) < 0) {
		return ret;
	}
	*oidp = oid;

	return E_SUCCESS;
}


int 
StorageAllocator::AllocateContainerVector(OsdSession* session)
{
	int                                                    ret;
	std::vector< ::osd::StorageProtocol::ContainerRequest> container_req_vec;
	std::vector<int>                                       rv;
	std::vector<int>::iterator                             rvi;

	::osd::StorageProtocol::ContainerRequest req;

	req.type = 1;
	req.num = 2;

	container_req_vec.push_back(req);

	ret = ipc_->call(osd::StorageProtocol::kAllocateContainerVector, 
	                 ipc_->id(), container_req_vec, rv);

	for (rvi = rv.begin(); rvi != rv.end(); rvi++) {
		printf("%d\n", *rvi);
	}
	return E_SUCCESS;
}


} // namespace client
} // namespace osd
