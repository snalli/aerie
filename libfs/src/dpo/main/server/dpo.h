#ifndef __STAMNOS_DPO_SERVER_DPO_LAYER_H
#define __STAMNOS_DPO_SERVER_DPO_LAYER_H

#include "ipc/ipc.h"
#include "dpo/main/server/dpo-opaque.h"

namespace dpo {
namespace server {


// Distributed Persistent Object Layer
class Dpo {
public:
	Dpo(::server::Ipc* ipc)
		: ipc_(ipc)
	{ }

	~Dpo();

	int Init();

	dpo::cc::server::HLockManager* hlckmgr() { return hlckmgr_; }
	dpo::cc::server::LockManager* lckmgr() { return lckmgr_; }
	StorageAllocator* salloc() { return salloc_; }
	Registry* registry() { return registry_; }

private:
	::server::Ipc*                  ipc_;
	dpo::cc::server::HLockManager*  hlckmgr_;
	dpo::cc::server::LockManager*   lckmgr_;
	StorageAllocator*               salloc_;
	Registry*                       registry_;
};

} // namespace server
} // namespace dpo

#endif // __STAMNOS_DPO_SERVER_DPO_LAYER_H
