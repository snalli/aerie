#ifndef __STAMNOS_SSA_SERVER_REGISTRY_H
#define __STAMNOS_SSA_SERVER_REGISTRY_H

#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>
#include "bcs/main/server/bcs.h"
#include "ssa/main/common/obj.h"


namespace ssa {
namespace server {


class Registry {
public:
	Registry(::server::Ipc* ipc);
	int Init();
	int Lookup(std::string name, ::ssa::common::ObjectId* oid);
	int Add(std::string name, ::ssa::common::ObjectId oid);
	int Remove(std::string name);

	class IpcHandlers {
	public:
		int Register(Registry* registry);
		
		int Lookup(unsigned int clt, const std::string name, ::ssa::common::ObjectId &r);
		int Add(unsigned int clt, const std::string name, ::ssa::common::ObjectId oid, int &r);
		int Remove(unsigned int clt, const std::string name, int &r);
	
	private:
		Registry* registry_;
	};

private:
	pthread_mutex_t                                mutex_;
	std::map<std::string, ::ssa::common::ObjectId> map_;
	::server::Ipc*                                 ipc_;
	IpcHandlers                                    ipc_handlers_;
};

} // namespace server
} // namespace ssa

#endif // __STAMNOS_SSA_SERVER_REGISTRY_H
