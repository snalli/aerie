#include <string>
#include "common/errno.h"
#include "ipc/ipc.h"
#include "server/fsmgr.h"
#include "server/fs_factory.h"
#include "pxfs/common/fs_protocol.h"


//FIXME: do we pass session? or client descriptor which has session?

namespace server {

FileSystemManager::FileSystemManager(Ipc* ipc, dpo::server::Dpo* dpo)
	: ipc_(ipc),
	  dpo_(dpo)
{
	fs_factory_map_.set_empty_key(0);
	fstype_str_to_id_map_.set_empty_key("");
}


int
FileSystemManager::Init()
{
	return ipc_handlers_.Register(this);
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
FileSystemManager::CreateFileSystem(Session* session, const char* target, 
                                    int fs_type, unsigned int flags) 
{
	FileSystemFactoryMap::iterator it;
	FileSystemFactory*             fs_factory; 
	dpo::common::ObjectId          oid;
	int                            ret;

	it = fs_factory_map_.find(fs_type);
	if (it == fs_factory_map_.end()) {
		return -1;
	}
	fs_factory = it->second;
	
	if ((ret = fs_factory->Make(session, &oid)) < 0) {
		return ret;
	}

	return E_SUCCESS;
}


int 
FileSystemManager::CreateFileSystem(Session* session, const char* target, 
                                    const char* fs_type, unsigned int flags)
{
	int fs_type_id = FSTypeStrToId(fs_type);

	return CreateFileSystem(session, target, fs_type_id, flags);
}


int 
FileSystemManager::MountFileSystem(Session* session, const char* target, 
                                   int fs_type, unsigned int flags,
                                   dpo::common::ObjectId* oid) 
{
	FileSystemFactoryMap::iterator it;
	FileSystemFactory*             fs_factory; 

	it = fs_factory_map_.find(fs_type);
	if (it == fs_factory_map_.end()) {
		return -1;
	}

	printf("MOUNT MOUNT MOUNT\n");
	
	return -1;
}


int 
FileSystemManager::MountFileSystem(Session* session, const char* target, 
                                   const char* fs_type, unsigned int flags, 
                                   dpo::common::ObjectId* oid)
{
	int fs_type_id = FSTypeStrToId(fs_type);

	return MountFileSystem(session, target, fs_type_id, flags, oid);
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
                                                 unsigned int flags, 
												 int& r)
{
	int ret;

	//FIXME: we pass NULL as session
	if ((ret = module_->CreateFileSystem(NULL, target.c_str(), fs_type.c_str(), 
	                                     flags)) < 0) 
	{
		return -ret;
	}
	return 0;
}


int
FileSystemManager::IpcHandlers::MountFileSystem(unsigned int clt, 
                                                std::string target, 
                                                std::string fs_type, 
                                                unsigned int flags,
                                                dpo::common::ObjectId& r)
{
	int                    ret;
	dpo::common::ObjectId  tmp_oid;

	//FIXME: we pass NULL as session
	if ((ret = module_->MountFileSystem(NULL, target.c_str(), fs_type.c_str(), 
	                                    flags, &tmp_oid)) < 0) 
	{
		return -ret;
	}
	r = tmp_oid;
	return 0;
}



/*
int 
FileSystemObjectManager::LoadSuperBlock(Session* session, dpo::common::ObjectId oid, 
                                        int fs_type, SuperBlock** sbp)
{
	SuperBlockFactoryMap::iterator it;
	SuperBlockFactory*             sb_factory; 

	it = sb_factory_map_.find(fs_type);
	if (it == sb_factory_map_.end()) {
		return -1;
	}
	sb_factory = it->second;
	return sb_factory->Load(session, oid, sbp);
}


int 
FileSystemObjectManager::LoadSuperBlock(Session* session, dpo::common::ObjectId oid, 
                                        const char* fs_type, SuperBlock** sbp)
{
	int fs_type_id = FSTypeStrToId(fs_type);
	
	return LoadSuperBlock(session, oid, fs_type_id, sbp);
}
*/


} // namespace server
