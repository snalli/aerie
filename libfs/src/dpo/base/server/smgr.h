#ifndef __STAMNOS_DPO_BASE_SERVER_STORAGE_MANAGER_H
#define __STAMNOS_DPO_BASE_SERVER_STORAGE_MANAGER_H

#include <vector>
#include "rpc/rpc.h"
#include "dpo/base/common/storage_protocol.h"

namespace dpo {
namespace server {


class StorageManager {
public:
	explicit StorageManager(rpcs* serverp);
	int Init(rpcs* rpc_server);

	int AllocateContainerVector(int clt, std::vector< ::dpo::StorageProtocol::ContainerRequest> container_request_vector, int& result);
private:


};



} // namespace server
} // namespace dpo



#endif // __STAMNOS_DPO_BASE_SERVER_MANAGER_H
