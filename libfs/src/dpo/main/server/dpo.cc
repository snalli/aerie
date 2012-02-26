#include "dpo/main/server/dpo.h"
#include "common/errno.h"
#include "common/debug.h"
#include "dpo/main/server/salloc.h"
#include "dpo/main/server/hlckmgr.h"
#include "dpo/main/server/registry.h"


namespace dpo {
namespace server {

/*
int
Dpo::Open(const char* pathname)
{

}
*/

int
Dpo::Init()
{
	dbg_log(DBG_INFO, "Initializing DPO layer\n");

	if ((hlckmgr_ = new ::dpo::cc::server::HLockManager(ipc_)) == NULL) {
		return -E_NOMEM;
	}
	hlckmgr_->Init();
	if ((salloc_ = new StorageAllocator(ipc_, this)) == NULL) {
		delete hlckmgr_;
		return -E_NOMEM;
	}
	salloc_->Init();
	if ((registry_ = new Registry(ipc_)) == NULL) {
		delete salloc_;
		delete hlckmgr_;
	}
	registry_->Init();
	return E_SUCCESS;
}


Dpo::~Dpo()
{
	delete registry_;
	delete salloc_;
	delete hlckmgr_;
}

} // namespace server
} // namespace dpo
