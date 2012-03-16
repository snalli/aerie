#ifndef __STAMNOS_DPO_MAIN_SERVER_STORAGE_ALLOCATOR_H
#define __STAMNOS_DPO_MAIN_SERVER_STORAGE_ALLOCATOR_H

#include <vector>
#include "ipc/ipc.h"
#include "dpo/main/server/obj.h"
#include "dpo/main/server/container.h"
#include "ipc/main/server/cltdsc.h"
#include "dpo/main/server/dpo-opaque.h"
#include "dpo/main/common/storage_protocol.h"
#include "dpo/containers/set/container.h"
#include "spa/pool/pool.h"

namespace server {
class Session;  // forward declaration
} // namespace client


namespace dpo {
namespace server {


/**
 * \brief Allocates containers from a storage pool.
 * 
 * We keep a single storage allocator for each storage pool.
 *
 */
class StorageAllocator {
	typedef google::dense_hash_map< ::dpo::common::ObjectType, ::dpo::server::ContainerAbstractFactory*> ObjectType2Factory; 
public:

	StorageAllocator(::server::Ipc* ipc);
	int Init();
	int Load(StoragePool* pool);
	int Make(StoragePool* pool);
	//static int Close(StorageAllocator* salloc);

	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(DpoSession* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocateExtent(DpoSession* session, size_t size, void** ptr);
	int AllocateContainer(DpoSession* session, int type, int num, ::dpo::StorageProtocol::ContainerReply& r);

	int RegisterBaseTypes();
	int RegisterType(::dpo::common::ObjectType type_id, ::dpo::server::ContainerAbstractFactory* objfactory);
	class IpcHandlers {
	public:
		int Register(StorageAllocator* salloc);

		int AllocateContainer(int clt, int type, int num, ::dpo::StorageProtocol::ContainerReply& r);
		int AllocateContainerVector(int clt, std::vector< ::dpo::StorageProtocol::ContainerRequest> container_request_vector, std::vector<int>& result);

	private:
		StorageAllocator* salloc_;
	}; 

private:
	::server::Ipc*                                                        ipc_;
	StorageAllocator::IpcHandlers                                         ipc_handlers_;
	pthread_mutex_t                                                       mutex_;
	StoragePool*                                                          pool_;
	dpo::containers::server::SetContainer<dpo::common::ObjectId>::Object* freeset_;
	ObjectType2Factory                                                    objtype2factory_map_; 
};


} // namespace server
} // namespace dpo


#endif // __STAMNOS_DPO_MAIN_SERVER_ALLOCATOR_H
