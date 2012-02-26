#ifndef __STAMNOS_DPO_MAIN_SERVER_STORAGE_ALLOCATOR_H
#define __STAMNOS_DPO_MAIN_SERVER_STORAGE_ALLOCATOR_H

#include <vector>
#include "ipc/ipc.h"
#include "ipc/main/server/cltdsc.h"
#include "dpo/main/server/dpo-opaque.h"
#include "dpo/main/common/storage_protocol.h"
#include "dpo/main/common/obj.h"

namespace server {
class Session;  // forward declaration
} // namespace client


namespace dpo {
namespace server {


class StorageAllocatorManager; // forward declaration

/**
 * \brief Allocates containers from a storage pool.
 * 
 * We keep a single storage allocator for each storage pool.
 *
 */
class StorageAllocator {
friend class StorageAllocatorManager;
public:
	static int Open(const char* pathname, size_t size, int flags, StorageAllocator** salloc);
	static int Close(StorageAllocator* salloc);

	StorageAllocator(::server::Ipc* ipc, dpo::server::Dpo* dpo);
	int Init();

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
	// private interface for use by the StorageAllocatorManager to prevent accidental 
	// construction of multiple storage allocators on top of a single storage pool
	::server::Ipc*                ipc_;
	dpo::server::Dpo*             dpo_;
	StorageAllocator::IpcHandlers ipc_handlers_;
	int descriptor_count_; // deprecated
};


// holds per client storage session information
struct StorageClientDescriptor: public ::server::ClientDescriptorTemplate<StorageClientDescriptor> {
public:
	StorageClientDescriptor() {
		printf("StorageDescriptor: CONSTRUCTOR: %p\n", this);
	}
	int id;
	int cap;
	
};





} // namespace server
} // namespace dpo



#endif // __STAMNOS_DPO_MAIN_SERVER_ALLOCATOR_H
