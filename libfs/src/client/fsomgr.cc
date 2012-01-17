#include <string>
#include "client/fsomgr.h"
#include "client/sb_factory.h"
#include "client/inode_factory.h"

namespace client {

FileSystemObjectManager::FileSystemObjectManager(rpcc* rpc_client, 
                                                 unsigned int principal_id)
{
	sb_factory_map_.set_empty_key(0);
	fstype_str_to_id_map_.set_empty_key("");
}


void
FileSystemObjectManager::Register(int type_id, const char* type_str, 
                                  SuperBlockFactory* sb_factory, 
                                  InodeFactory* inode_factory)
{
	sb_factory_map_[type_id] = sb_factory;
	fstype_str_to_id_map_[std::string(type_str)] = type_id;
}


void
FileSystemObjectManager::Register(SuperBlockFactory* sb_factory, 
                                  InodeFactory* inode_factory)
{
	return Register(sb_factory->TypeID(), sb_factory->TypeStr().c_str(), sb_factory, inode_factory); 
}


void 
FileSystemObjectManager::Unregister(int type_id)
{
	// TODO
	return;
}


int 
FileSystemObjectManager::FSTypeStrToId(const char* fs_type)
{
	FSTypeStrToIdMap::iterator it;

	it = fstype_str_to_id_map_.find(fs_type);
	if (it == fstype_str_to_id_map_.end()) {
		return -1;
	}
	return it->second;
}


int 
FileSystemObjectManager::CreateSuperBlock(Session* session, int fs_type, 
                                          SuperBlock** sbp)
{
	SuperBlockFactoryMap::iterator it;
	SuperBlockFactory*             sb_factory; 

	it = sb_factory_map_.find(fs_type);
	if (it == sb_factory_map_.end()) {
		return -1;
	}
	sb_factory = it->second;
	return sb_factory->Make(session, sbp);
}


int 
FileSystemObjectManager::CreateSuperBlock(Session* session, const char* fs_type, 
                                          SuperBlock** sbp)
{
	int fs_type_id = FSTypeStrToId(fs_type);
	
	return CreateSuperBlock(session, fs_type_id, sbp);
}


int 
FileSystemObjectManager::AllocInode(Session* session, Inode* parent, 
                                    int type, Inode** ipp)
{
	// TODO


}


} // namespace client
