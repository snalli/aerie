#ifndef __STAMNOS_OSD_MAIN_SERVER_STORAGE_ALLOCATOR_H
#define __STAMNOS_OSD_MAIN_SERVER_STORAGE_ALLOCATOR_H

#include <map>
#include <vector>
#include "bcs/bcs.h"
#include "osd/main/server/obj.h"
#include "osd/main/server/container.h"
#include "osd/main/server/osd-opaque.h"
#include "osd/main/common/storage_protocol.h"
#include "osd/containers/set/container.h"
#include "scm/pool/pool.h"

namespace server {
class Session;  // forward declaration
} // namespace client


namespace osd {
namespace server {

typedef osd::containers::server::SetContainer<osd::common::ObjectId>::Object ObjectIdSet;


struct AclObjectPair {
	osd::common::AclIdentifier acl_id;
	osd::common::ObjectId      oid;
};


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
	DescriptorPool(osd::common::ObjectId set_oid)
	{		  	
		set_obj_ = ObjectIdSet::Load(set_oid);
	}
	
	static int Create(OsdSession* session, osd::common::ObjectId set_oid, DescriptorPool** poolp);
	
	int Load(OsdSession* session);
	int AllocateContainer(OsdSession* session, StorageAllocator* salloc, int type, osd::common::ObjectId* oid);
	int AllocateExtent(OsdSession* session, StorageAllocator* salloc, size_t nbytes, osd::common::ExtentId* eid);

private:
	ObjectIdSet*                     set_obj_;
	std::list<ExtentDescriptor>      extent_list_;
	std::list<ContainerDescriptor>   container_list_[16]; // support 16 types: 0-15
};



/**
 * \brief Allocates containers from a storage pool.
 * 
 * We keep a single storage allocator for each storage pool.
 *
 */


class StorageAllocator {
	typedef google::dense_hash_map< ::osd::common::ObjectType, ::osd::server::ContainerAbstractFactory*> ObjectType2Factory; 
	typedef osd::containers::server::SetContainer<AclObjectPair>::Object FreeSet;
	typedef std::multimap<osd::common::AclIdentifier, ObjectIdSet*> FreeMap;
	typedef google::dense_hash_map< ::osd::common::AclIdentifier, DescriptorPool*> AclPoolMap;

public:
	static int Load(::server::Ipc* ipc, StoragePool* pool, StorageAllocator** sallocp);

	StorageAllocator(::server::Ipc* ipc, StoragePool* pool, FreeSet* freeset);
	int Init();
	/** Shutdowns the allocator, and destroys the object if is was created using one of the factory methods */
	int Close();

	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(OsdSession* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int CreateObjectIdSet(OsdSession* session, osd::common::AclIdentifier acl_id, ObjectIdSet** obj_set);
	int AllocateObjectIdSet(OsdSession* session, osd::common::AclIdentifier acl_id, osd::common::ObjectId* set_oid);
	int AllocateObjectIdSet(OsdSession* session, osd::common::AclIdentifier acl_id, ::osd::StorageProtocol::ContainerReply& reply);
	int AllocateExtent(OsdSession* session, size_t size, int flags, void** ptr);
	int AllocateExtentIntoSet(OsdSession* session, ObjectIdSet* set, int size, int count);
	int AllocateExtentFromSet(OsdSession* session, osd::common::ObjectId set_oid, osd::common::ExtentId eid, int index_hint);
	int AllocateContainer(OsdSession* session, osd::common::AclIdentifier acl_id, int type, osd::common::ObjectId* oidp);
	int AllocateContainerIntoSet(OsdSession* session, ObjectIdSet* set, int type, int count);
	int AllocateContainerFromSet(OsdSession* session, osd::common::ObjectId set_oid, osd::common::ObjectId oid, int index_hint);
	int GetDescriptorPool(OsdSession* session, osd::common::AclIdentifier acl_id, DescriptorPool** poolp);
	int FreeContainer(OsdSession* session, osd::common::ObjectId oid);
	int FreeExtent(OsdSession* session, osd::common::ExtentId eid);
	int RegisterBaseTypes();
	int RegisterType(::osd::common::ObjectType type_id, ::osd::server::ContainerAbstractFactory* objfactory);
	class IpcHandlers {
	public:
		int Register(StorageAllocator* salloc);

		int AllocateObjectIdSet(int clt, osd::common::AclIdentifier acl_id, ::osd::StorageProtocol::ContainerReply& r);
		int AllocateContainerIntoSet(int clt, int set_capability, int type, int num, int& reply);
		int AllocateExtentIntoSet(int clt, int set_capability, int size, int num, int& reply);
		int AllocateContainerVector(int clt, std::vector< ::osd::StorageProtocol::ContainerRequest> container_request_vector, std::vector<int>& result);

	private:
		StorageAllocator* salloc_;
	}; 

private:
	int LoadFreeMap();

	::server::Ipc*                   ipc_;
	StorageAllocator::IpcHandlers    ipc_handlers_;
	pthread_mutex_t                  mutex_;
	StoragePool*                     pool_;
	FreeSet*                         freeset_; // pointer to the persistent free set
	FreeMap                          freemap_; // pointer to the persistent free set
	ObjectType2Factory               objtype2factory_map_; 
	bool                             can_commit_suicide_;
	AclPoolMap                       aclpoolmap_; // per ACL descriptor pools for local use only
	
	//FIXME: the following is used to keep a global list for lazy recycling 
	//storage. it's flaky and should be fixed. it may also lose storage in
	//case of crash because storage stays here after a server is done with
	//its journal
	std::list< ::osd::common::ObjectId>   container_list_[16]; // support 16 types: 0-15
};


} // namespace server
} // namespace osd


#endif // __STAMNOS_OSD_MAIN_SERVER_ALLOCATOR_H
