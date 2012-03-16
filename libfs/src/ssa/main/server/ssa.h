#ifndef __STAMNOS_SSA_SERVER_SSA_LAYER_H
#define __STAMNOS_SSA_SERVER_SSA_LAYER_H

#include "ipc/ipc.h"
#include "spa/pool/pool.h"
#include "ssa/main/server/ssa-opaque.h"
#include "ssa/containers/super/container.h"

namespace ssa {
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

	ssa::cc::server::HLockManager* hlckmgr() { return hlckmgr_; }
	ssa::cc::server::LockManager* lckmgr() { return lckmgr_; }
	StorageAllocator* salloc() { return salloc_; }
	Registry* registry() { return registry_; }
	int Load(const char* source, unsigned int flags);
	int Make(const char* target, unsigned int flags);
	int Close();
	ssa::containers::server::SuperContainer::Object* super_obj() { return super_obj_; }
	StoragePool* pool() { return pool_; }

private:
	::server::Ipc*                                    ipc_;
	ssa::cc::server::HLockManager*                    hlckmgr_;
	ssa::cc::server::LockManager*                     lckmgr_;
	StorageAllocator*                                 salloc_;
	Registry*                                         registry_;
	StoragePool*                                      pool_;
	ssa::containers::server::SuperContainer::Object*  super_obj_;
};


} // namespace server
} // namespace ssa

#endif // __STAMNOS_SSA_SERVER_SSA_LAYER_H
