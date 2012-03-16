#ifndef __STAMNOS_SSA_MAIN_SERVER_STORAGE_ALLOCATOR_H
#define __STAMNOS_SSA_MAIN_SERVER_STORAGE_ALLOCATOR_H

#include <vector>
#include "ipc/ipc.h"
#include "ssa/main/server/obj.h"
#include "ssa/main/server/container.h"
#include "ipc/main/server/cltdsc.h"
#include "ssa/main/server/ssa-opaque.h"
#include "ssa/main/common/storage_protocol.h"
#include "ssa/containers/set/container.h"
#include "spa/pool/pool.h"

namespace server {
class Session;  // forward declaration
} // namespace client


namespace ssa {
namespace server {


/**
 * \brief Allocates containers from a storage pool.
 * 
 * We keep a single storage allocator for each storage pool.
 *
 */
class StorageAllocator {
	typedef google::dense_hash_map< ::ssa::common::ObjectType, ::ssa::server::ContainerAbstractFactory*> ObjectType2Factory; 
public:

	StorageAllocator(::server::Ipc* ipc);
	int Init();
	int Load(StoragePool* pool);
	int Make(StoragePool* pool);
	//static int Close(StorageAllocator* salloc);

	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(SsaSession* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocateExtent(SsaSession* session, size_t size, void** ptr);
	int AllocateContainer(SsaSession* session, int type, int num, ::ssa::StorageProtocol::ContainerReply& r);

	int RegisterBaseTypes();
	int RegisterType(::ssa::common::ObjectType type_id, ::ssa::server::ContainerAbstractFactory* objfactory);
	class IpcHandlers {
	public:
		int Register(StorageAllocator* salloc);

		int AllocateContainer(int clt, int type, int num, ::ssa::StorageProtocol::ContainerReply& r);
		int AllocateContainerVector(int clt, std::vector< ::ssa::StorageProtocol::ContainerRequest> container_request_vector, std::vector<int>& result);

	private:
		StorageAllocator* salloc_;
	}; 

private:
	::server::Ipc*                                                        ipc_;
	StorageAllocator::IpcHandlers                                         ipc_handlers_;
	pthread_mutex_t                                                       mutex_;
	StoragePool*                                                          pool_;
	ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object* freeset_;
	ObjectType2Factory                                                    objtype2factory_map_; 
};


} // namespace server
} // namespace ssa


#endif // __STAMNOS_SSA_MAIN_SERVER_ALLOCATOR_H
