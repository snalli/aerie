#ifndef __STAMNOS_FS_CLIENT_SESSION_H
#define __STAMNOS_FS_CLIENT_SESSION_H

#include "dpo/base/client/hlckmgr.h"
#include "dpo/base/client/lckmgr.h"
#include "dpo/base/client/stm.h"

namespace client {

class StorageManager;

class Session {
public:
	Session(dpo::cc::client::LockManager* lckmgr, 
	        dpo::cc::client::HLockManager* hlckmgr, 
	        StorageManager* smgr)
		: lckmgr_(lckmgr),
		  hlckmgr_(hlckmgr),
		  smgr_(smgr)
	{ }

	dpo::cc::client::LockManager*    lckmgr_;
	dpo::cc::client::HLockManager*   hlckmgr_;
	client::StorageManager*          smgr_;
	dpo::stm::client::Transaction*   tx_;
};

} // namespace client

#endif // __STAMNOS_FS_CLIENT_SESSION_H
