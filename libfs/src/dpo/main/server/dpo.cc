#include "dpo/main/server/dpo.h"
#include "common/errno.h"
#include "common/debug.h"
#include "dpo/main/server/smgr.h"
#include "dpo/main/server/hlckmgr.h"


namespace dpo {
namespace server {

int
Dpo::Init()
{
	dbg_log(DBG_INFO, "Initializing DPO layer\n");

	if ((hlckmgr_ = new ::dpo::cc::server::HLockManager(ipc_)) == NULL) {
		return -E_NOMEM;
	}
	hlckmgr_->Init();
	if ((smgr_ = new StorageManager(ipc_)) == NULL) {
		delete hlckmgr_;
		return -E_NOMEM;
	}
	smgr_->Init();
	return E_SUCCESS;
}


Dpo::~Dpo()
{
	delete smgr_;
	delete hlckmgr_;
}

} // namespace client
} // namespace dpo
