#include "osd/main/client/stsystem.h"
#include "common/errno.h"
#include "spa/pool/pool.h"
#include "osd/main/client/salloc.h"
#include "osd/main/client/omgr.h"
#include "osd/main/client/lckmgr.h"
#include "osd/main/client/hlckmgr.h"
#include "osd/main/client/registry.h"
#include "osd/main/client/shbuf.h"

namespace osd {
namespace client {

int
StorageSystem::Init()
{
	if ((lckmgr_ = new ::osd::cc::client::LockManager(ipc_)) == NULL) {
		return -E_NOMEM;
	}
	if ((hlckmgr_ = new ::osd::cc::client::HLockManager(lckmgr_)) == NULL) {
		delete lckmgr_;
		return -E_NOMEM;
	}
	// don't change order: ObjectManager expects a storage system with 
	// initialized lock managers
	if ((omgr_ = new ObjectManager(this)) == NULL) {
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
StorageSystem::Open(const char* source, unsigned int flags)
{
	int ret;

	if ((ret = StoragePool::Open(source, &pool_)) < 0) {
		return ret;
	}
	if ((ret = salloc_->Load(pool_)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


int 
StorageSystem::Mount(const char* source, const char* target, unsigned int flags,
                     StorageSystemDescriptor& desc)
{
	int ret;

	if ((shbuf_ = new OsdSharedBuffer(ipc_, desc.shbuf_dsc_)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = shbuf_->Map()) < 0) {
		return ret;
	}
	if ((ret = Open(source, flags)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


int 
StorageSystem::Mount(const char* source, unsigned int flags,
                     StorageSystemDescriptor& desc)
{
	return Mount(source, NULL, flags, desc);
}


int 
StorageSystem::Mount(const char* source, const char* target, unsigned int flags)
{
	int                               ret;
	StorageSystemProtocol::MountReply mntrep;
	
	if (source == NULL) {
		return -E_INVAL;
	}
	if ((ret = ipc_->call(StorageSystemProtocol::kMount,
	                      ipc_->id(), std::string(source), 
	                      flags, mntrep)) < 0) 
	{
		return -E_IPC;
	}
	if (ret > 0) {
		return -ret;
	}
	return Mount(source, target, flags, mntrep.desc_);
}


int 
StorageSystem::Mount(const char* source, unsigned int flags)
{
	return Mount(source, NULL, flags);
}


int 
StorageSystem::Close()
{

}


StorageSystem::~StorageSystem()
{
	delete hlckmgr_;
	delete lckmgr_;
	hlckmgr_ = NULL; 
	lckmgr_ = NULL;
}

} // namespace client
} // namespace osd
