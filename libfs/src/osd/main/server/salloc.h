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
#include "spa/pool/pool.h"

namespace server {
class Session;  // forward declaration
} // namespace client


namespace osd {
namespace server {


struct AclObjectPair {
	osd::common::AclIdentifier acl_id;
	osd::common::ObjectId      oid;
};


/**
 * \brief Allocates containers from a storage pool.
 * 
 * We keep a single storage allocator for each storage pool.
 *
 */


class StorageAllocator {
	typedef google::dense_hash_map< ::osd::common::ObjectType, ::osd::server::ContainerAbstractFactory*> ObjectType2Factory; 
	typedef osd::containers::server::SetContainer<osd::common::ObjectId>::Object ObjectIdSet;
	typedef osd::containers::server::SetContainer<AclObjectPair>::Object FreeSet;
	typedef std::multimap<osd::common::AclIdentifier, ObjectIdSet*> FreeMap;

public:
	static int Load(::server::Ipc* ipc, StoragePool* pool, StorageAllocator** sallocp);

	StorageAllocator(::server::Ipc* ipc, StoragePool* pool, FreeSet* freeset);
	int Init();
	/** Shutdowns the allocator, and destroys the object if is was created using one of the factory methods */
	int Close();

	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(OsdSession* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocateExtent(OsdSession* session, size_t size, int flags, void** ptr);
	int CreateContainerSet(OsdSession* session, osd::common::AclIdentifier acl_id, ObjectIdSet** obj_set);
	int AllocateContainerSet(OsdSession* session, osd::common::AclIdentifier acl_id, ::osd::StorageProtocol::ContainerReply& reply);
	int AllocateContainer(OsdSession* session, ObjectIdSet* set, int type, int count, int& reply);

	int RegisterBaseTypes();
	int RegisterType(::osd::common::ObjectType type_id, ::osd::server::ContainerAbstractFactory* objfactory);
	class IpcHandlers {
	public:
		int Register(StorageAllocator* salloc);

		int AllocateContainerSet(int clt, osd::common::AclIdentifier acl_id, ::osd::StorageProtocol::ContainerReply& r);
		int AllocateContainer(int clt, int set_capability, int type, int num, int& reply);
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
};


} // namespace server
} // namespace osd


#endif // __STAMNOS_OSD_MAIN_SERVER_ALLOCATOR_H
