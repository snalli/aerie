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


int
DescriptorPool::Load(OsdSession* session)
{
	osd::common::ObjectId   oid;
	osd::common::ExtentId   eid;
	osd::common::Object*    obj;

	extent_list_.clear();
	for (int i = 0; i < 16; i++) {
		container_list_[i].clear();
	}

	for (int i = 0; i < set_obj_->Size(); i++) {
		set_obj_->Read(session, i, &oid);
		if (oid.type() == osd::containers::T_EXTENT) {
			eid = osd::common::ExtentId(oid);
			extent_list_.push_back(ExtentDescriptor(eid, i));
		} else {
			container_list_[oid.type()].push_back(ContainerDescriptor(oid, i));
		}
	}
	return E_SUCCESS;
}


int
DescriptorPool::LoadFromLast(OsdSession* session)
{
	osd::common::ObjectId   oid;
	osd::common::ExtentId   eid;
	osd::common::Object*    obj;

	for (int i = last_offset_; i < set_obj_->Size(); i++) {
		set_obj_->Read(session, i, &oid);
		if (oid.type() == osd::containers::T_EXTENT) {
			eid = osd::common::ExtentId(oid);
			extent_list_.push_back(ExtentDescriptor(eid, i));
		} else {
			container_list_[oid.type()].push_back(ContainerDescriptor(oid, i));
		}
	}
	last_offset_ = set_obj_->Size();
	return E_SUCCESS;
}


int
DescriptorPool::Create(::client::Ipc* ipc, OsdSession* session, 
                      osd::common::AclIdentifier acl_id, 
                      DescriptorPool** poolp)
{
	::osd::StorageProtocol::ContainerReply  reply;
	DescriptorPool*                         pool;
	int                                     ret;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_salloc), 
	        "[%d] Allocate a pool of containers with ACL %d\n", ipc->id(), acl_id);
	
	if ((ret = ipc->call(osd::StorageProtocol::kAllocateObjectIdSet, 
	                     ipc->id(), acl_id, reply)) < 0) {
		return ret;
	} else if (ret > 0) {
		return -ret;
	}
	if ((pool = new DescriptorPool(reply.index, reply.oid)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = pool->LoadFromLast(session)) < 0) {
		return ret;
	}
	*poolp = pool;
		
	return E_SUCCESS;
}


int 
DescriptorPool::AllocateContainer(::client::Ipc* ipc, OsdSession* session, int type, osd::common::ObjectId* oid)
{
	int r;
	int ret;

	if (container_list_[type].empty()) {
		if ((ret = ipc->call(osd::StorageProtocol::kAllocateContainerIntoSet, 
							 ipc->id(), capability_, type, 1024, r)) < 0) {
			return ret;
		} else if (ret > 0) {
			return -ret;
		}
		if ((ret = LoadFromLast(session)) < 0) {
			return ret;
		}
	}
	ContainerDescriptor& front = container_list_[type].front();
	*oid = front.oid_;
	session->journal() << Publisher::Message::LogicalOperation::AllocContainer(capability_, front.oid_, front.index_);
	container_list_[type].pop_front();
	return E_SUCCESS;
}


// the api allows multiple size extents.
// currently the implementation provides only 4K size extents
int
DescriptorPool::AllocateExtent(::client::Ipc* ipc, OsdSession* session, 
                               size_t nbytes, osd::common::ExtentId* eid)
{
	int ret;
	int r;

	if (extent_list_.empty()) {
		if ((ret = ipc->call(osd::StorageProtocol::kAllocateExtentIntoSet, 
							 ipc->id(), capability_, 4096, 1024, r)) < 0) {
			return ret;
		} else if (ret > 0) {
			return -ret;
		}
		if ((ret = LoadFromLast(session)) < 0) {
			return ret;
		}
	}
	ExtentDescriptor& front = extent_list_.front();
	*eid = front.eid_;
	session->journal() << osd::Publisher::Message::ContainerOperation::AllocateExtent(capability_, front.eid_, front.index_);
	extent_list_.pop_front();
	if (eid->u64() == 0x80faa81000) {
		printf("ALLOCATION\n");
	}
	return E_SUCCESS;
}


int 
StorageAllocator::Load(StoragePool* pool) 
{
	pool_ = pool;
	return E_SUCCESS;
}


int 
StorageAllocator::GetDescriptorPool(OsdSession* session, osd::common::AclIdentifier acl_id, 
                                    DescriptorPool** poolp)
{
	int                  ret;
	AclPoolMap::iterator it;
	DescriptorPool*       pool;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_salloc), 
	        "[%d] Allocate container set of ACL %d\n", ipc_->id(), acl_id);
	
	if ((it = aclpoolmap_.find(acl_id)) == aclpoolmap_.end()) {
		if ((ret = DescriptorPool::Create(ipc_, session, acl_id, &pool)) < 0) {
			return ret;
		}
		aclpoolmap_.insert(std::pair<osd::common::AclIdentifier, DescriptorPool*>(acl_id, pool));
	} else {
		pool = it->second;	
	}
	*poolp = pool;
	
	return E_SUCCESS;
}


int
StorageAllocator::AllocateExtent(OsdSession* session, osd::common::AclIdentifier acl_id, 
                                 size_t nbytes, osd::common::ExtentId* eidp, bool has_lock)
{
	int                   ret;
	DescriptorPool*       pool;
	osd::common::ExtentId eid;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_salloc), 
	        "[%d] Allocate extent of size %lu\n", ipc_->id(), nbytes);
	
	if (!has_lock) {
		pthread_mutex_lock(&mutex_);
	}
	if ((ret = GetDescriptorPool(session, acl_id, &pool)) < 0) {
		goto done;
	}
	if ((ret = pool->AllocateExtent(ipc_, session, nbytes, &eid)) < 0) {
		goto done;
	}
	*eidp = eid;
	ret = E_SUCCESS;
done:	
	if (!has_lock) {
		pthread_mutex_unlock(&mutex_);
	}
	return E_SUCCESS;
}


int
StorageAllocator::AllocateExtent(OsdSession* session, osd::common::AclIdentifier acl_id, size_t nbytes, int flags, void** ptr)
{
	int                   ret;
	osd::common::ExtentId eid;
	
	pthread_mutex_lock(&mutex_);
	if (flags & kMetadata) {
		// The container data structures may call us to allocate storage
		// for metadata. because we don't physically construct at the client
		// we give them private (volatile) memory instead. 
		*ptr = malloc(nbytes);
		ret = E_SUCCESS;
	} else {
		ret = AllocateExtent(session, acl_id, nbytes, &eid, true);
		*ptr = eid.addr();
	}
	pthread_mutex_unlock(&mutex_);
	return ret;
}


int
StorageAllocator::AllocateExtent(OsdSession* session, size_t nbytes, int flags, void** ptr)
{
	return AllocateExtent(session, 0, nbytes, flags, ptr);
}


int 
StorageAllocator::AllocateContainer(OsdSession* session, osd::common::AclIdentifier acl_id, 
                                    int type, osd::common::ObjectId* oidp)
{
	int                   ret;
	DescriptorPool*       pool;
	osd::common::ObjectId oid;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_salloc), 
	        "[%d] Allocate container of type %d\n", ipc_->id(), type);

	pthread_mutex_lock(&mutex_);

	if ((ret = GetDescriptorPool(session, acl_id, &pool)) < 0) {
		goto done;
	}
	if ((ret = pool->AllocateContainer(ipc_, session, type, &oid)) < 0) {
		goto done;
	}
	*oidp = oid;
	ret = E_SUCCESS;
done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


int 
StorageAllocator::AllocateContainerVector(OsdSession* session)
{
	int                                                    ret;
	std::vector< ::osd::StorageProtocol::ContainerRequest> container_req_vec;
	std::vector<int>                                       rv;
	std::vector<int>::iterator                             rvi;
	::osd::StorageProtocol::ContainerRequest               req;

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
