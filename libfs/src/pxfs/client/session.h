#ifndef __STAMNOS_FS_CLIENT_SESSION_H
#define __STAMNOS_FS_CLIENT_SESSION_H

#include "ssa/main/client/ssa.h"
#include "ssa/main/client/ssa-opaque.h"
#include "ssa/main/client/stm.h"
#include "ssa/main/client/journal.h"

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
	{ 
		journal_ = new ssa::client::Journal(this);
	}

	ssa::client::StorageAllocator* salloc() { return salloc_; }
	ssa::client::Journal* journal() { return journal_; }

	ssa::client::StorageSystem*      stsystem_;
	ssa::cc::client::LockManager*    lckmgr_;
	ssa::cc::client::HLockManager*   hlckmgr_;
	ssa::client::StorageAllocator*   salloc_;
	ssa::client::ObjectManager*      omgr_;
	ssa::stm::client::Transaction*   tx_;
	ssa::client::Journal*            journal_;
};


} // namespace client

#endif // __STAMNOS_FS_CLIENT_SESSION_H
