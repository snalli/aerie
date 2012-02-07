#ifndef __STAMNOS_DPO_BASE_SERVER_STORAGE_MANAGER_H
#define __STAMNOS_DPO_BASE_SERVER_STORAGE_MANAGER_H


namespace dpo
namespace server {


class StorageManager {
public:
	explicit StorageManager(rpcs* serverp);
	int Init(rpcs* rpc_server);

private:


};



} // namespace server
} // namespace dpo



#endif // __STAMNOS_DPO_BASE_SERVER_MANAGER_H
