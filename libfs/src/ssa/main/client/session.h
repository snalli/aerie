#ifndef __STAMNOS_SSA_CLIENT_SESSION_H
#define __STAMNOS_SSA_CLIENT_SESSION_H

#include "ssa/main/client/journal.h"
#include "ssa/main/client/stm.h"
#include "ssa/main/client/stsystem.h"

namespace ssa {
namespace client {

class SsaSession {
public:
	SsaSession(ssa::client::StorageSystem* stsystem)
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
	ssa::client::StorageSystem*      stsystem() { return stsystem_; }

	ssa::client::StorageSystem*      stsystem_;
	ssa::cc::client::LockManager*    lckmgr_;
	ssa::cc::client::HLockManager*   hlckmgr_;
	ssa::client::StorageAllocator*   salloc_;
	ssa::client::ObjectManager*      omgr_;
	ssa::stm::client::Transaction*   tx_;
	ssa::client::Journal*            journal_;
};

} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_SESSION_H
