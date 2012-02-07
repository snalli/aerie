#ifndef __STAMNOS_FS_CLIENT_SESSION_H
#define __STAMNOS_FS_CLIENT_SESSION_H

#include "dpo/base/client/dpo.h"
#include "dpo/base/client/stm.h"

namespace dpo { 
namespace client { 
class ObjectManager;   // forward declaration
} // namespace client
} // namespace dpo


class StorageManager;

namespace client {

class Session {
public:
	Session(dpo::client::DpoLayer* dpo_layer)
		: lckmgr_(dpo_layer->lckmgr()),
		  hlckmgr_(dpo_layer->hlckmgr()),
		  smgr_(dpo_layer->smgr()),
		  omgr_(dpo_layer->omgr())
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


	dpo::cc::client::LockManager*    lckmgr_;
	dpo::cc::client::HLockManager*   hlckmgr_;
	dpo::client::StorageManager*     smgr_;
	dpo::client::ObjectManager*      omgr_;
	dpo::stm::client::Transaction*   tx_;
};


} // namespace client

#endif // __STAMNOS_FS_CLIENT_SESSION_H
