#ifndef __STAMNOS_DPO_SERVER_REGISTRY_H
#define __STAMNOS_DPO_SERVER_REGISTRY_H

#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>
#include "ipc/ipc.h"


namespace dpo {
namespace server {


class Registry {
public:
	Registry(::server::Ipc* ipc);
	int Init();
	int Lookup(std::string name, uint64_t*);
	int Add(std::string name, uint64_t);
	int Remove(std::string name);

	class IpcHandlers {
	public:
		IpcHandlers(Registry* registry)
			: registry_(registry)
		{ }

		int Init();
		
		int Lookup(int clt, const std::string name, unsigned long long &r);
	private:
		Registry* registry_;
	};

private:
	pthread_mutex_t                 mutex_;
	std::map<std::string, uint64_t> map_;
	::server::Ipc*                  ipc_;
	IpcHandlers                     ipc_handlers_;
};

} // namespace server
} // namespace dpo

#endif // __STAMNOS_DPO_SERVER_REGISTRY_H
