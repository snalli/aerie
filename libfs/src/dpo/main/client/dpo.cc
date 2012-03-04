#include "dpo/main/client/dpo.h"
#include "common/errno.h"
#include "sal/pool/pool.h"
#include "dpo/main/client/salloc.h"
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
} // namespace dpo
