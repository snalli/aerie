#include <string>
#include "server/fsmgr.h"
#include "server/fs_factory.h"

namespace server {

FileSystemManager::FileSystemManager()
{
	fs_factory_map_.set_empty_key(0);
	fstype_str_to_id_map_.set_empty_key("");
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
                                    int fs_type) 
{
	FileSystemFactoryMap::iterator it;
	FileSystemFactory*             fs_factory; 

	it = fs_factory_map_.find(fs_type);
	if (it == fs_factory_map_.end()) {
		return -1;
	}
	fs_factory = it->second;
	return fs_factory->Make(session);
}


int 
FileSystemManager::CreateFileSystem(Session* session, const char* target, 
                                    const char* fs_type)
{
	int fs_type_id = FSTypeStrToId(fs_type);
	
	return CreateFileSystem(session, target, fs_type_id);
}

//extern dpo::server::Dpo* dpo_layer;


int
FileSystemManager::CreateFileSystem(int clt, const char* target, const char* fs_type, int& r)
{
	//Server session(dpo_layer);
	
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
