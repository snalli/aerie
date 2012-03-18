#include <string>
#include "common/errno.h"
#include "bcs/bcs.h"
#include "ssa/main/server/ssa.h"
#include "ssa/main/server/sessionmgr.h"
#include "spa/pool/pool.h"
#include "pxfs/common/fs_protocol.h"
#include "pxfs/server/fs.h"
#include "pxfs/server/session.h"
#include "pxfs/server/server.h"


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
FileSystem::Make(const char* target, size_t nblocks, size_t block_size, int flags) 
{
	int ret;

	if ((ret = ssa::server::StorageSystem::Make(target, flags)) < 0) {
		return ret;
	}
	return E_SUCCESS;
}


int 
FileSystem::Load(Ipc* ipc, const char* source, unsigned int flags, FileSystem** fsp)
{
	int                         ret;
	ssa::server::StorageSystem* storage_system;
	FileSystem*                 fs;

	if ((ret = ssa::server::StorageSystem::Load(ipc, source, flags, &storage_system)) < 0) {
		return ret;
	}
	if ((fs = new FileSystem(ipc, storage_system)) == NULL) {
		storage_system->Close();
		return -E_NOMEM;
	}
	if ((ret = fs->Init()) < 0) {
		return ret;
	}
	*fsp = fs;
	return E_SUCCESS;
}


int 
FileSystem::Mount(int clt, const char* source, const char* target, 
                  unsigned int flags, ssa::common::ObjectId* oid) 
{
	int                            ret;
	Session*                       session;

	pthread_mutex_lock(&mutex_);

	if ((ret = Server::Instance()->session_manager()->Create(clt, &session)) < 0) {
		return -ret;
	}
	*oid = storage_system_->super_obj()->oid();
	ret = E_SUCCESS;

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


int 
FileSystem::IpcHandlers::Register(FileSystem* module)
{
	module_ = module;
	module_->ipc_->reg(::FileSystemProtocol::kMount, this, 
	                   &::server::FileSystem::IpcHandlers::Mount);

	return E_SUCCESS;
}


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
