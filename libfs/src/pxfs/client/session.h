#ifndef __STAMNOS_FS_CLIENT_SESSION_H
#define __STAMNOS_FS_CLIENT_SESSION_H

#include "ssa/main/client/ssa.h"
#include "ssa/main/client/ssa-opaque.h"
#include "ssa/main/client/stm.h"


class StorageAllocator;

namespace client {

class Session {
public:
	Session(ssa::client::StorageSystem* stsystem)
		: lckmgr_(stsystem->lckmgr()),
		  hlckmgr_(stsystem->hlckmgr()),
		  salloc_(stsystem->salloc()),
		  omgr_(stsystem->omgr()),
		  stsystem_(stsystem)
	{ }
	
	Session(ssa::cc::client::LockManager* lckmgr, 
	        ssa::cc::client::HLockManager* hlckmgr)
		: lckmgr_(lckmgr),
		  hlckmgr_(hlckmgr),
		  salloc_(NULL),
		  omgr_(NULL)
	{ }

	Session(ssa::cc::client::LockManager* lckmgr, 
	        ssa::cc::client::HLockManager* hlckmgr,
	        ssa::client::StorageAllocator* salloc,
	        ssa::client::ObjectManager* omgr)
		: lckmgr_(lckmgr),
		  hlckmgr_(hlckmgr),
		  salloc_(salloc),
		  omgr_(omgr)
	{ }


	ssa::client::StorageAllocator* salloc() { return stsystem_->salloc(); }

	ssa::client::StorageSystem*      stsystem_;
	ssa::cc::client::LockManager*    lckmgr_;
	ssa::cc::client::HLockManager*   hlckmgr_;
	ssa::client::StorageAllocator*   salloc_;
	ssa::client::ObjectManager*      omgr_;
	ssa::stm::client::Transaction*   tx_;
};


} // namespace client

#endif // __STAMNOS_FS_CLIENT_SESSION_H
