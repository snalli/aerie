#include "dpo/base/server/dpo.h"
#include "common/errno.h"
#include "common/debug.h"
#include "dpo/base/server/smgr.h"
#include "dpo/base/server/hlckmgr.h"


namespace dpo {
namespace server {

int
DpoLayer::Init()
{
	dbg_log(DBG_INFO, "Initializing DPO layer\n");

	if ((hlckmgr_ = new ::dpo::cc::server::HLockManager(rpc_server_)) == NULL) {
		return -E_NOMEM;
	}
	hlckmgr_->Init();
	if ((smgr_ = new StorageManager(rpc_server_)) == NULL) {
		delete hlckmgr_;
		return -E_NOMEM;
	}
	smgr_->Init();
	return E_SUCCESS;
}


DpoLayer::~DpoLayer()
{
	delete smgr_;
	delete hlckmgr_;
}

} // namespace client
} // namespace dpo
