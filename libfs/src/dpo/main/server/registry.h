#ifndef __STAMNOS_DPO_SERVER_REGISTRY_H
#define __STAMNOS_DPO_SERVER_REGISTRY_H

#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>
#include "ipc/ipc.h"
#include "dpo/main/common/obj.h"


namespace dpo {
namespace server {


class Registry {
public:
	Registry(::server::Ipc* ipc);
	int Init();
	int Lookup(std::string name, ::dpo::common::ObjectId* oid);
	int Add(std::string name, ::dpo::common::ObjectId oid);
	int Remove(std::string name);

	class IpcHandlers {
	public:
		int Register(Registry* registry);
		
		int Lookup(unsigned int clt, const std::string name, ::dpo::common::ObjectId &r);
		int Add(unsigned int clt, const std::string name, ::dpo::common::ObjectId oid, int &r);
		int Remove(unsigned int clt, const std::string name, int &r);
	
	private:
		Registry* registry_;
	};

private:
	pthread_mutex_t                                mutex_;
	std::map<std::string, ::dpo::common::ObjectId> map_;
	::server::Ipc*                                 ipc_;
	IpcHandlers                                    ipc_handlers_;
};

} // namespace server
} // namespace dpo

#endif // __STAMNOS_DPO_SERVER_REGISTRY_H
