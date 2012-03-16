#include "ssa/main/client/ssa.h"
#include "common/errno.h"
#include "spa/pool/pool.h"
#include "ssa/main/client/salloc.h"
#include "ssa/main/client/omgr.h"
#include "ssa/main/client/lckmgr.h"
#include "ssa/main/client/hlckmgr.h"
#include "ssa/main/client/registry.h"

namespace ssa {
namespace client {

int
Dpo::Init()
{
	if ((lckmgr_ = new ::ssa::cc::client::LockManager(ipc_)) == NULL) {
		return -E_NOMEM;
	}
	if ((hlckmgr_ = new ::ssa::cc::client::HLockManager(lckmgr_)) == NULL) {
		delete lckmgr_;
		return -E_NOMEM;
	}
	if ((omgr_ = new ObjectManager(lckmgr_, hlckmgr_)) == NULL) {
		delete hlckmgr_;
		delete lckmgr_;
		return -E_NOMEM;
	}
	if ((salloc_ = new StorageAllocator(ipc_)) == NULL) {
		delete omgr_;
		delete hlckmgr_;
		delete lckmgr_;
		return -E_NOMEM;
	}
	if ((registry_ = new Registry(ipc_)) == NULL) {
		delete omgr_;
		delete hlckmgr_;
		delete lckmgr_;
		delete salloc_;
		return -E_NOMEM;
	}
	return E_SUCCESS;
}


int 
Dpo::Open(const char* source, unsigned int flags)
{
	int ret;

	if ((ret = StoragePool::Open(source, &pool_)) < 0) {
		return E_SUCCESS;
	}
	return salloc_->Load(pool_);
}


int 
Dpo::Close()
{

}


Dpo::~Dpo()
{
	delete hlckmgr_;
	delete lckmgr_;
	hlckmgr_ = NULL; 
	lckmgr_ = NULL;
}

} // namespace client
} // namespace ssa
