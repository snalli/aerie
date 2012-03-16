#include <string>
#include "common/errno.h"
#include "ipc/ipc.h"
#include "ssa/main/server/registry.h"
#include "ssa/main/server/salloc.h"
#include "pxfs/common/fs_protocol.h"
#include "spa/pool/pool.h"
#include "server/fsmgr.h"
#include "server/fs_factory.h"
#include "server/fs.h"
#include "server/session.h"
#include "server/server.h"
#include "server/sessionmgr.h"


namespace server {

FileSystemManager::FileSystemManager(Ipc* ipc, ssa::server::Dpo* ssa)
	: ipc_(ipc),
	  ssa_(ssa),
	  mounted_fs_(NULL)
{
	fs_factory_map_.set_empty_key(0);
	fstype_str_to_id_map_.set_empty_key("");
	pthread_mutex_init(&mutex_, NULL);
}


int
FileSystemManager::Init()
{
	if (ipc_) {
		return ipc_handlers_.Register(this);
	}
	return E_SUCCESS;
}


void
FileSystemManager::Register(int type_id, const char* type_str, 
                            FileSystemFactory* fs_factory)
{
	fs_factory_map_[type_id] = fs_factory;
	fstype_str_to_id_map_[std::string(type_str)] = type_id;
}


void
FileSystemManager::Register(FileSystemFactory* fs_factory)
{
	return Register(fs_factory->TypeID(), fs_factory->TypeStr().c_str(), 
	                fs_factory); 
}


void 
FileSystemManager::Unregister(int type_id)
{
	// TODO
	return;
}


int 
FileSystemManager::FSTypeStrToId(const char* fs_type)
{
	FSTypeStrToIdMap::iterator it;

	it = fstype_str_to_id_map_.find(fs_type);
	if (it == fstype_str_to_id_map_.end()) {
		return -1;
	}
	return it->second;
}


int 
FileSystemManager::CreateFileSystem(const char* target, int fs_type, 
                                    size_t nblocks, size_t block_size, int flags) 
{
	FileSystemFactoryMap::iterator it;
	FileSystemFactory*             fs_factory; 
	ssa::common::ObjectId          oid;
	int                            ret;
	StoragePool*                   pool;

	pthread_mutex_lock(&mutex_);

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

	ret =  E_SUCCESS;
done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


int 
FileSystemManager::CreateFileSystem(const char* target, const char* fs_type, 
                                    size_t nblocks, size_t block_size, int flags)
{
	int fs_type_id = FSTypeStrToId(fs_type);

	return CreateFileSystem(target, fs_type_id, nblocks, block_size, flags);
}


int 
FileSystemManager::MountFileSystem(int clt, const char* source, 
                                   const char* target, 
                                   int fs_type, unsigned int flags,
                                   ssa::common::ObjectId* oid) 
{
	FileSystemFactoryMap::iterator it;
	FileSystemFactory*             fs_factory; 
	FileSystem*                    fs;
	ssa::common::ObjectId          tmp_oid;
	int                            ret;
	uint64_t                       identity;
	StoragePool*                   pool;
	Session*                       session;

	pthread_mutex_lock(&mutex_);
	
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
	ret = E_SUCCESS;

done:
	pthread_mutex_unlock(&mutex_);
	return ret;
}


int 
FileSystemManager::MountFileSystem(int clt, const char* source, 
                                   const char* target, 
                                   const char* fs_type, unsigned int flags, 
                                   ssa::common::ObjectId* oid)
{
	int fs_type_id = FSTypeStrToId(fs_type);

	return MountFileSystem(clt, source, target, fs_type_id, flags, oid);
}


int 
FileSystemManager::IpcHandlers::Register(FileSystemManager* module)
{
	module_ = module;
	module_->ipc_->reg(::FileSystemProtocol::kCreateFileSystem, this, 
	                   &::server::FileSystemManager::IpcHandlers::CreateFileSystem);
	module_->ipc_->reg(::FileSystemProtocol::kMountFileSystem, this, 
	                   &::server::FileSystemManager::IpcHandlers::MountFileSystem);

	return E_SUCCESS;
}


int
FileSystemManager::IpcHandlers::CreateFileSystem(unsigned int clt, 
                                                 std::string target, 
                                                 std::string fs_type, 
												 unsigned int nblocks,
												 unsigned int block_size,
                                                 unsigned int flags,
                                                 ssa::common::ObjectId& unused)
{
	int                   ret;
	ssa::common::ObjectId tmp_oid;
	
	if ((ret = module_->CreateFileSystem(target.c_str(), fs_type.c_str(), 
	                                     nblocks, block_size, (int) flags)) < 0) {
		return -ret;
	}
	return 0;
}


/**
 * \todo Currently we support a single storage pool per server so we ignore the 
 * source string provided by the client. 
 */
int
FileSystemManager::IpcHandlers::MountFileSystem(unsigned int clt, 
                                                std::string source, 
                                                std::string target, 
                                                std::string fs_type, 
                                                unsigned int flags,
                                                ssa::common::ObjectId& r)
{
	int                   ret;
	ssa::common::ObjectId tmp_oid;
	
	if ((ret = module_->MountFileSystem(clt, source.c_str(), target.c_str(), 
	                                    fs_type.c_str(), flags, &tmp_oid)) < 0)	{
		return -ret;
	}
	r = tmp_oid;
	return 0;
}


} // namespace server
