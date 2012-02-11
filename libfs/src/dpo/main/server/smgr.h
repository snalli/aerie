#ifndef __STAMNOS_DPO_BASE_SERVER_STORAGE_MANAGER_H
#define __STAMNOS_DPO_BASE_SERVER_STORAGE_MANAGER_H

#include <vector>
#include "ipc/ipc.h"
#include "dpo/main/common/storage_protocol.h"

namespace dpo {
namespace server {


class StorageManager {
public:
	explicit StorageManager(::server::Ipc* ipc);
	int Init();

	int AllocateContainer(int clt, int type, int num, ::dpo::StorageProtocol::Capability& cap);
	int AllocateContainerVector(int clt, std::vector< ::dpo::StorageProtocol::ContainerRequest> container_request_vector, std::vector<int>& result);

private:
	::server::Ipc* ipc_;
};



} // namespace server
} // namespace dpo



#endif // __STAMNOS_DPO_BASE_SERVER_MANAGER_H
