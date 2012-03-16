#include <string>
#include "common/errno.h"
#include "ipc/ipc.h"
#include "ssa/main/server/registry.h"
#include "ssa/main/server/salloc.h"
#include "spa/pool/pool.h"
#include "pxfs/common/fs_protocol.h"
#include "pxfs/server/fs.h"
#include "pxfs/server/session.h"
#include "pxfs/server/server.h"
#include "pxfs/server/sessionmgr.h"


namespace server {

FileSystem::FileSystem(Ipc* ipc, ssa::server::StorageSystem* storage_system)
	: ipc_(ipc),
	  storage_system_(storage_system)
{
	pthread_mutex_init(&mutex_, NULL);
}


int
FileSystem::Init()
{
	if (ipc_) {
		return ipc_handlers_.Register(this);
	}
	return E_SUCCESS;
}


int 
FileSystem::Create(const char* target, size_t nblocks, size_t block_size, int flags) 
{
	ssa::common::ObjectId          oid;
	int                            ret;
	StoragePool*                   pool;

	pthread_mutex_lock(&mutex_);
#if 0
	it = fs_factory_map_.find(fs_type);
	if (it == fs_factory_map_.end()) {
		ret = -E_UNKNOWNFS;
		goto done;
	}
	fs_factory = it->second;

	if ((ret = ssa_->Make(target, flags)) < 0) {
		goto done;
	}
	if ((ret = fs_factory->Make(ssa_, nblocks, block_size, flags)) < 0) {
		ssa_->Close();
		goto done;
	}
	ssa_->Close();
#endif
	ret =  E_SUCCESS;
done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


int 
FileSystem::Mount(int clt, const char* source, const char* target, 
                  unsigned int flags, ssa::common::ObjectId* oid) 
{
	FileSystem*                    fs;
	ssa::common::ObjectId          tmp_oid;
	int                            ret;
	uint64_t                       identity;
	StoragePool*                   pool;
	Session*                       session;

	pthread_mutex_lock(&mutex_);

#if 0
	if ((ret = StoragePool::Identity(source, &identity)) < 0) {
		goto done;
	}
	
	if ((it = fs_factory_map_.find(fs_type)) == fs_factory_map_.end()) {
			ret = -E_UNKNOWNFS;
	}
	fs_factory = it->second;

	// if a filesystem is not already loaded then load it.
	// we allow a single file system instance
	if (ssa_->pool()) {
		if (ssa_->pool()->Identity() == identity) {
			fs = mounted_fs_;
		} else {
			ret = -E_NOTEMPTY;
			goto done;
		}
	} else {
		if ((ret = ssa_->Load(source, flags)) < 0) {
			goto done;
		}
		if ((ret = fs_factory->Load(ssa_, flags, &fs)) < 0) {
			ssa_->Close();
			goto done;
		}
		mounted_fs_ = fs;
	}

	if ((ret = Server::Instance()->session_manager()->Create(clt, &session)) < 0) {
		return -ret;
	}

	*oid = fs->superblock();
#endif	
	ret = E_SUCCESS;

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


int 
FileSystem::IpcHandlers::Register(FileSystem* module)
{
	module_ = module;
	module_->ipc_->reg(::FileSystemProtocol::kCreate, this, 
	                   &::server::FileSystem::IpcHandlers::Create);
	module_->ipc_->reg(::FileSystemProtocol::kMount, this, 
	                   &::server::FileSystem::IpcHandlers::Mount);

	return E_SUCCESS;
}


int
FileSystem::IpcHandlers::Create(unsigned int clt, std::string target, 
                                unsigned int nblocks, unsigned int block_size,
                                unsigned int flags, ssa::common::ObjectId& unused)
{
	int                   ret;
	ssa::common::ObjectId tmp_oid;
	
	if ((ret = module_->Create(target.c_str(), nblocks, block_size,
	                           (int) flags)) < 0) {
		return -ret;
	}
	return 0;
}


/**
 * \todo Currently we support a single storage pool per server so we ignore the 
 * source string provided by the client. 
 */
int
FileSystem::IpcHandlers::Mount(unsigned int clt, std::string source, 
                               std::string target, unsigned int flags,
                               ssa::common::ObjectId& r)
{
	int                   ret;
	ssa::common::ObjectId tmp_oid;
	
	if ((ret = module_->Mount(clt, source.c_str(), target.c_str(), 
	                          flags, &tmp_oid)) < 0) {
		return -ret;
	}
	r = tmp_oid;
	return 0;
}


} // namespace server
