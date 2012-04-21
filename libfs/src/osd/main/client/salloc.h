#ifndef __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H
#define __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <typeinfo>
#include <google/dense_hash_set>
#include "bcs/bcs-opaque.h"
#include "osd/main/common/obj.h"
#include "osd/main/client/osd-opaque.h"
#include "osd/containers/set/container.h"
#include "scm/pool/pool.h"


namespace osd {
namespace client {

typedef osd::containers::client::SetContainer<osd::common::ObjectId>::Object ObjectIdSet;


class ExtentDescriptor {
public:
	ExtentDescriptor(osd::common::ExtentId eid, int index)
		: eid_(eid),
		  index_(index)
	{ }

	osd::common::ExtentId eid_;
	int                   index_; // Index in the persistent container set; 
	                              // to be given to the server as a hint
};


class ContainerDescriptor {
public:
	ContainerDescriptor(osd::common::ObjectId oid, int index)
		: oid_(oid),
		  index_(index)
	{ }

	osd::common::ObjectId oid_;
	int                   index_; // Index in the persistent container set; 
	                              // to be given to the server as a hint
};


// organizes free storage descriptors of the same ACL in per type free lists 
class DescriptorPool {
public:
	DescriptorPool(int capability, osd::common::ObjectId oid)
		: capability_(capability),
		  last_offset_(0)
	{		  	
		set_obj_ = ObjectIdSet::Load(oid);
	}
	
	
	static int Create(::client::Ipc* ipc, OsdSession* session, osd::common::AclIdentifier acl_id, DescriptorPool** poolp);
	
	int LoadFromLast(OsdSession* session);
	int Load(OsdSession* session);
	int AllocateContainer(::client::Ipc* ipc, OsdSession* session, int type, osd::common::ObjectId* oid);
	int AllocateExtent(::client::Ipc* ipc, OsdSession* session, size_t nbytes, osd::common::ExtentId* eid);

private:
	int                              capability_; // index in the capability table of the server
	ObjectIdSet*                     set_obj_;
	int                              last_offset_;  // the last offset of the set we loaded into the pool
	std::list<ExtentDescriptor>      extent_list_;
	std::list<ContainerDescriptor>   container_list_[16]; // support 16 types: 0-15
};


class StorageAllocator {
	typedef google::dense_hash_set< ::osd::common::ObjectId, ::osd::common::ObjectIdHashFcn > ObjectIdSet;
	typedef google::dense_hash_map< ::osd::common::AclIdentifier, DescriptorPool*> AclPoolMap;
public:
	
	StorageAllocator(::client::Ipc* ipc)
		: ipc_(ipc)
	{ 
		pthread_mutex_init(&mutex_, NULL);
		aclpoolmap_.set_empty_key(INT_MAX);
		aclpoolmap_.set_deleted_key(INT_MAX-1);
	}	
	int Load(StoragePool* pool);

	int AllocateExtent(OsdSession* session, size_t nbytes, int flags, void** ptr);
	int AllocateExtent(OsdSession* session, osd::common::AclIdentifier acl_id, size_t nbytes, int flags, void** ptr);
	int AllocateExtent(OsdSession* session, osd::common::AclIdentifier acl_id, size_t nbytes, osd::common::ExtentId* eidp);
	int AllocateContainer(OsdSession* session, osd::common::AclIdentifier acl_id, int type, osd::common::ObjectId* oid);
	int AllocateContainerVector(OsdSession* session);
	int GetDescriptorPool(OsdSession* session, osd::common::AclIdentifier acl_id, DescriptorPool** poolp);

private:
	pthread_mutex_t mutex_;
	::client::Ipc*  ipc_;
	StoragePool*    pool_;
	AclPoolMap      aclpoolmap_;
};

} // namespace client
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H
