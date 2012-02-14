#include "dpo/main/client/dpo.h"
#include "common/errno.h"
#include "dpo/main/client/smgr.h"
#include "dpo/main/client/omgr.h"
#include "dpo/main/client/lckmgr.h"
#include "dpo/main/client/hlckmgr.h"
#include "dpo/main/client/registry.h"

namespace dpo {
namespace client {

int
Dpo::Init()
{
	if ((lckmgr_ = new ::dpo::cc::client::LockManager(ipc_)) == NULL) {
		return -E_NOMEM;
	}
	if ((hlckmgr_ = new ::dpo::cc::client::HLockManager(lckmgr_)) == NULL) {
		delete lckmgr_;
		return -E_NOMEM;
	}
	if ((omgr_ = new ObjectManager(lckmgr_, hlckmgr_)) == NULL) {
		delete hlckmgr_;
		delete lckmgr_;
		return -E_NOMEM;
	}
	if ((smgr_ = new StorageManager(ipc_)) == NULL) {
		delete omgr_;
		delete hlckmgr_;
		delete lckmgr_;
		return -E_NOMEM;
	}
	if ((registry_ = new Registry(ipc_)) == NULL) {
		delete omgr_;
		delete hlckmgr_;
		delete lckmgr_;
		delete smgr_;
		return -E_NOMEM;
	}
	return E_SUCCESS;
}


Dpo::~Dpo()
{
	delete hlckmgr_;
	delete lckmgr_;
	hlckmgr_ = NULL; 
	lckmgr_ = NULL;
}

} // namespace client
} // namespace dpo
