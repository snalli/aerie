#ifndef __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H
#define __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <typeinfo>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_set>
#include "bcs/bcs-opaque.h"
#include "osd/main/common/obj.h"
#include "osd/main/client/osd-opaque.h"
#include "osd/containers/set/container.h"
#include "spa/pool/pool.h"


namespace osd {
namespace client {


class FreeContainerDescriptor {
public:
	FreeContainerDescriptor(osd::common::ObjectId oid, int index)
		: oid_(oid),
		  index_(index)
	{ }

	osd::common::ObjectId oid_;
	int                   index_; // Index in the persistent container set; 
	                              // to be given to the server as a hint
};


// organizes containers of the same ACL in per type free lists 
class ContainerPool {
public:
	ContainerPool(int capability, osd::common::ObjectId oid)
		: capability_(capability),
		  last_offset_(0)
	{		  	
		set_obj_ = osd::containers::client::SetContainer<osd::common::ObjectId>::Object::Load(oid);
	}
	
	
	static int Create(::client::Ipc* ipc, OsdSession* session, osd::common::AclIdentifier acl_id, ContainerPool** poolp);
	
	int LoadFromLast(OsdSession* session);
	int Allocate(::client::Ipc* ipc, OsdSession* session, int type, osd::common::ObjectId* oid);

private:
	int                                capability_; // index in the capability table of the server
	osd::containers::client::SetContainer<osd::common::ObjectId>::Object* set_obj_;
	int                                last_offset_;  // the last offset of the set we loaded into the pool
	std::list<FreeContainerDescriptor> freelist_[16]; // support 16 types: 0-15
};


class StorageAllocator {
	typedef google::dense_hash_set< ::osd::common::ObjectId, ::osd::common::ObjectIdHashFcn > ObjectIdSet;
	typedef google::dense_hash_map< ::osd::common::AclIdentifier, ContainerPool*> AclPoolMap;
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
	int AllocateContainer(OsdSession* session, osd::common::AclIdentifier acl_id, int type, osd::common::ObjectId* oid);
	int AllocateContainerVector(OsdSession* session);
	int GetContainerPool(OsdSession* session, osd::common::AclIdentifier acl_id, ContainerPool** poolp);

private:
	pthread_mutex_t mutex_;
	::client::Ipc*  ipc_;
	StoragePool*    pool_;
	AclPoolMap      aclpoolmap_;
};

} // namespace client
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_STORAGE_ALLOCATOR_H
