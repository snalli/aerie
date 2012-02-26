#ifndef __STAMNOS_FS_CLIENT_SESSION_H
#define __STAMNOS_FS_CLIENT_SESSION_H

#include "dpo/main/client/dpo.h"
#include "dpo/main/client/dpo-opaque.h"
#include "dpo/main/client/stm.h"


class StorageAllocator;

namespace client {

class Session {
public:
	Session(dpo::client::Dpo* dpo)
		: lckmgr_(dpo->lckmgr()),
		  hlckmgr_(dpo->hlckmgr()),
		  salloc_(dpo->salloc()),
		  omgr_(dpo->omgr()),
		  dpo_(dpo)
	{ }
	
	Session(dpo::cc::client::LockManager* lckmgr, 
	        dpo::cc::client::HLockManager* hlckmgr)
		: lckmgr_(lckmgr),
		  hlckmgr_(hlckmgr),
		  salloc_(NULL),
		  omgr_(NULL)
	{ }

	Session(dpo::cc::client::LockManager* lckmgr, 
	        dpo::cc::client::HLockManager* hlckmgr,
	        dpo::client::StorageAllocator* salloc,
	        dpo::client::ObjectManager* omgr)
		: lckmgr_(lckmgr),
		  hlckmgr_(hlckmgr),
		  salloc_(salloc),
		  omgr_(omgr)
	{ }


	dpo::client::StorageAllocator* salloc() { return dpo_->salloc(); }

	dpo::client::Dpo*                dpo_;
	dpo::cc::client::LockManager*    lckmgr_;
	dpo::cc::client::HLockManager*   hlckmgr_;
	dpo::client::StorageAllocator*     salloc_;
	dpo::client::ObjectManager*      omgr_;
	dpo::stm::client::Transaction*   tx_;
};


} // namespace client

#endif // __STAMNOS_FS_CLIENT_SESSION_H
