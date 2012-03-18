#ifndef __STAMNOS_SSA_SERVER_STORAGE_SYSTEM_H
#define __STAMNOS_SSA_SERVER_STORAGE_SYSTEM_H

#include "bcs/bcs.h"
#include "spa/pool/pool.h"
#include "ssa/main/server/ssa-opaque.h"
#include "ssa/containers/super/container.h"

namespace ssa {
namespace server {


/** \brief Encapsulates the SSA components composing a storage system */
class StorageSystem {
public:
	// factory methods
	static int Load(::server::Ipc* ipc, const char* source, unsigned int flags, StorageSystem** storage_system_ptr);
	static int Make(const char* target, unsigned int flags);
	
	StorageSystem(::server::Ipc* ipc, StoragePool* pool)
		: ipc_(ipc),
		  pool_(pool),
		  salloc_(NULL),
		  can_commit_suicide_(false)
	{ }

	~StorageSystem();

	int Init();
	int Close();

	ssa::cc::server::HLockManager* hlckmgr() { return hlckmgr_; }
	ssa::cc::server::LockManager* lckmgr() { return lckmgr_; }
	StorageAllocator* salloc() { return salloc_; }
	Registry* registry() { return registry_; }
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
	bool                                              can_commit_suicide_;
};


} // namespace server
} // namespace ssa

#endif // __STAMNOS_SSA_SERVER_SSA_LAYER_H
