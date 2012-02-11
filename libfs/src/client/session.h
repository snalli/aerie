#ifndef __STAMNOS_FS_CLIENT_SESSION_H
#define __STAMNOS_FS_CLIENT_SESSION_H

#include "dpo/main/client/dpo.h"
#include "dpo/main/client/stm.h"

namespace dpo { 
namespace client { 
class ObjectManager;   // forward declaration
} // namespace client
} // namespace dpo


class StorageManager;

namespace client {

class Session {
public:
	Session(dpo::client::Dpo* dpo)
		: lckmgr_(dpo->lckmgr()),
		  hlckmgr_(dpo->hlckmgr()),
		  smgr_(dpo->smgr()),
		  omgr_(dpo->omgr()),
		  dpo_(dpo)
	{ }
	
	Session(dpo::cc::client::LockManager* lckmgr, dpo::cc::client::HLockManager* hlckmgr)
		: lckmgr_(lckmgr),
		  hlckmgr_(hlckmgr),
		  smgr_(NULL),
		  omgr_(NULL)
	{ }

	Session(dpo::cc::client::LockManager* lckmgr, 
	        dpo::cc::client::HLockManager* hlckmgr,
	        dpo::client::StorageManager* smgr,
	        dpo::client::ObjectManager* omgr)
		: lckmgr_(lckmgr),
		  hlckmgr_(hlckmgr),
		  smgr_(smgr),
		  omgr_(omgr)
	{ }

	dpo::client::Dpo*                dpo_;
	dpo::cc::client::LockManager*    lckmgr_;
	dpo::cc::client::HLockManager*   hlckmgr_;
	dpo::client::StorageManager*     smgr_;
	dpo::client::ObjectManager*      omgr_;
	dpo::stm::client::Transaction*   tx_;
};


} // namespace client

#endif // __STAMNOS_FS_CLIENT_SESSION_H
