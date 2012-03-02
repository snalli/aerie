#ifndef __STAMNOS_DPO_MAIN_SERVER_STORAGE_ALLOCATOR_H
#define __STAMNOS_DPO_MAIN_SERVER_STORAGE_ALLOCATOR_H

#include <vector>
#include "ipc/ipc.h"
#include "ipc/main/server/cltdsc.h"
#include "dpo/main/server/dpo-opaque.h"
#include "dpo/main/common/obj.h"
#include "dpo/main/common/storage_protocol.h"
#include "dpo/containers/set/container.h"
#include "sal/pool/pool.h"

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
public:

	StorageAllocator(::server::Ipc* ipc);
	int Init();
	int Load(StoragePool* pool);
	//static int Close(StorageAllocator* salloc);

	int Alloc(size_t nbytes, std::type_info const& typid, void** ptr);
	int Alloc(::server::Session* session, size_t nbytes, std::type_info const& typid, void** ptr);
	int AllocateRaw(::server::Session* session, size_t size, void** ptr);
	int AllocateContainer(::server::Session* session, int type, int num);
	
	class IpcHandlers {
	public:
		int Register(StorageAllocator* salloc);

		int AllocateContainer(int clt, int type, int num, int& r);
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
};


} // namespace server
} // namespace dpo


#endif // __STAMNOS_DPO_MAIN_SERVER_ALLOCATOR_H
