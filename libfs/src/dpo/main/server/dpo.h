#ifndef __STAMNOS_DPO_SERVER_DPO_LAYER_H
#define __STAMNOS_DPO_SERVER_DPO_LAYER_H

#include "ipc/ipc.h"
#include "sal/pool/pool.h"
#include "dpo/main/server/dpo-opaque.h"
#include "dpo/containers/super/container.h"

namespace dpo {
namespace server {


// Persistent Object Layer
class Dpo {
public:
	Dpo(::server::Ipc* ipc)
		: ipc_(ipc),
		  salloc_(NULL),
		  pool_(NULL)
	{ }

	~Dpo();

	int Init();

	dpo::cc::server::HLockManager* hlckmgr() { return hlckmgr_; }
	dpo::cc::server::LockManager* lckmgr() { return lckmgr_; }
	StorageAllocator* salloc() { return salloc_; }
	Registry* registry() { return registry_; }
	int Load(const char* source, unsigned int flags);
	int Make(const char* target, unsigned int flags);
	int Close();
	dpo::containers::server::SuperContainer::Object* super_obj() { return super_obj_; }
	StoragePool* pool() { return pool_; }

private:
	::server::Ipc*                                    ipc_;
	dpo::cc::server::HLockManager*                    hlckmgr_;
	dpo::cc::server::LockManager*                     lckmgr_;
	StorageAllocator*                                 salloc_;
	Registry*                                         registry_;
	StoragePool*                                      pool_;
	dpo::containers::server::SuperContainer::Object*  super_obj_;
};


} // namespace server
} // namespace dpo

#endif // __STAMNOS_DPO_SERVER_DPO_LAYER_H
