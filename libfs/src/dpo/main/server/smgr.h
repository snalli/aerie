#ifndef __STAMNOS_DPO_MAIN_SERVER_STORAGE_MANAGER_H
#define __STAMNOS_DPO_MAIN_SERVER_STORAGE_MANAGER_H

#include <vector>
#include "ipc/ipc.h"
#include "ipc/main/server/cltdsc.h"
#include "dpo/main/common/storage_protocol.h"

namespace server {
class Session;  // forward declaration
} // namespace client


namespace dpo {
namespace server {


class StorageManager {
public:
	explicit StorageManager(::server::Ipc* ipc);
	int Init();

	int AllocateRaw(::server::Session* session, size_t size, void** ptr);
	int AllocateContainer(int clt, int type, int num, ::dpo::StorageProtocol::Capability& cap);
	int AllocateContainerVector(int clt, std::vector< ::dpo::StorageProtocol::ContainerRequest> container_request_vector, std::vector<int>& result);

private:
	::server::Ipc* ipc_;
};


struct StorageDescriptor: public ::server::ClientDescriptorTemplate<StorageDescriptor> {
public:
	StorageDescriptor() {
		printf("StorageDescriptor: CONSTRUCTOR: %p\n", this);
	}
	int id;
	int cap;
	
};



} // namespace server
} // namespace dpo



#endif // __STAMNOS_DPO_MAIN_SERVER_MANAGER_H
