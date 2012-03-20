#include "ssa/main/server/ssa.h"
#include "common/errno.h"
#include "bcs/bcs.h"
#include "ssa/main/server/salloc.h"
#include "ssa/main/server/hlckmgr.h"
#include "ssa/main/server/registry.h"
#include "ssa/main/server/shbuf.h"
#include "ssa/containers/super/container.h"
#include "ssa/containers/name/container.h"
#include "ssa/containers/set/container.h"
#include "spa/pool/pool.h"
#include "spa/const.h"

namespace ssa {
namespace server {


int
StorageSystem::Init()
{
	int ret;

	if ((hlckmgr_ = new ::ssa::cc::server::HLockManager(ipc_)) == NULL) {
		return -E_NOMEM;
	}
	hlckmgr_->Init();
	if ((ret = StorageAllocator::Load(ipc_, pool_, &salloc_)) < 0) {
		delete hlckmgr_;
		return ret;
	}
	if ((registry_ = new Registry(ipc_)) == NULL) {
		salloc_->Close();
		delete hlckmgr_;
	}
	registry_->Init();
	if ((ret = ipc_->shbuf_manager()->RegisterSharedBufferType("SsaSharedBuffer", SsaSharedBuffer::Make)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


int
StorageSystem::Load(::server::Ipc* ipc, const char* source, 
                    unsigned int flags, StorageSystem** storage_system_ptr)
{
	int                ret;
	void*              b;
	StorageSystem*     storage_system;	
	StoragePool*       pool;
	::server::Session* session = NULL; // we need no journaling
	
	DBG_LOG(DBG_INFO, DBG_MODULE(server_storagesystem), 
	        "Load storage system %s\n", source);

	if ((ret = StoragePool::Open(source, &pool)) < 0) {
		return ret;
	}
	if ((b = pool->root()) == 0) {
		StoragePool::Close(pool);
		return -E_NOENT;
	}
	if ((storage_system = new StorageSystem(ipc, pool)) == NULL) {
		StoragePool::Close(pool);
		return -E_NOMEM;
	}
	if ((storage_system->super_obj_ = ssa::containers::server::SuperContainer::Object::Load(b)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = storage_system->Init()) < 0) {
		return ret;
	}
	storage_system->can_commit_suicide_ = true;
	*storage_system_ptr = storage_system;

	return E_SUCCESS;
}


/**
 * \brief Creates a storage system in the pool
 */
int 
StorageSystem::Make(const char* target, unsigned int flags)
{
	ssa::containers::server::SuperContainer::Object*                      super_obj;
	ssa::containers::server::NameContainer::Object*                       root_obj;
	ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object* set_obj;
	int                                                                   ret;
	char*                                                                 b;
	char*                                                                 buffer;
	SsaSession*                                                           session = NULL; // we need no journaling and storage allocator
	size_t                                                                master_extent_size;
	StoragePool*                                                          pool;
	
	if ((ret = StoragePool::Open(target, &pool)) < 0) {
		return ret;
	}
	
	// 1) create the superblock object/proxy,
	// 2) create the directory inode objext/proxy and set the root 
	//    of the superblock to point to the new directory inode.
	
	master_extent_size = 0;
	master_extent_size += sizeof(ssa::containers::server::NameContainer::Object);
	master_extent_size += sizeof(ssa::containers::server::SuperContainer::Object);
	master_extent_size += sizeof(ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object);
	if ((ret = pool->AllocateExtent(master_extent_size, (void**) &buffer)) < 0) {
		return ret;
	}
	pool->set_root((void*) buffer);

	// superblock 
	b = buffer;
	if ((super_obj = ssa::containers::server::SuperContainer::Object::Make(session, b)) == NULL) {
		return -E_NOMEM;
	}
	// root directory inode
	b += sizeof(ssa::containers::server::SuperContainer::Object);
	if ((root_obj = ssa::containers::server::NameContainer::Object::Make(session, b)) == NULL) {
		return -E_NOMEM;
	}
	super_obj->set_root(session, root_obj->oid());
	// storage container
	b += sizeof(ssa::containers::server::NameContainer::Object);
	if ((set_obj = ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object::Make(session, b)) == NULL) {
		return -E_NOMEM;
	}
	super_obj->set_freelist(session, set_obj->oid());
	b += sizeof(ssa::containers::server::SetContainer<ssa::common::ObjectId>::Object);
	assert(b < buffer + master_extent_size + 1); 
	return E_SUCCESS;
}


int
StorageSystem::Close()
{
	int ret;

	if ((ret = StoragePool::Close(pool_)) < 0) {
		return ret;
	}
	if (can_commit_suicide_) {
		delete this;
	}
	return E_SUCCESS;
}


StorageSystem::~StorageSystem()
{
	delete registry_;
	delete salloc_;
	delete hlckmgr_;
}

} // namespace server
} // namespace ssa
