/**
 * \file fsmgr.h
 *
 * \brief File System Object manager
 *
 * Manages file system logical objects: superblocks and inodes 
 *
 * It plays the role of a mediator to backend superblock and inode 
 * factory methods: each backend file system registers factory code with 
 * this manager to create superblocks and inodes. It can be
 * therefore seen as an abstract factory (but its interface
 * it's not exactly an AFT)
 *
 */

/**
 * This manager provides an interface to create and manipulate the 
 * logical objects of the file system: superblocks, inodes.
 * 
 * The front-end needs a way to create new inodes (directories, files)
 * 
 * Steps to create an inode:
 *  - 
 */

#ifndef __STAMNOS_CLIENT_FILE_SYSTEM_OBJECT_MANAGER_H
#define __STAMNOS_CLIENT_FILE_SYSTEM_OBJECT_MANAGER_H

#include <string>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "rpc/rpc.h"
#include "client/inode.h"

namespace client {

class SuperBlockFactory; // forward declaration
class InodeFactory;      // forward declaration
class Session;           // forward declaration

class FileSystemObjectManager {
	typedef google::dense_hash_map<int, SuperBlockFactory*> SuperBlockFactoryMap;
	typedef google::dense_hash_map<int, InodeFactory*>      InodeFactoryMap;
	typedef google::dense_hash_map<std::string, int>        FSTypeStrToIdMap;

public:
	FileSystemObjectManager(rpcc* rpc_client, unsigned int principal_id);
	void Register(int type_id, const char* type_str, SuperBlockFactory* sb_factory, InodeFactory* inode_factory);
	void Register(SuperBlockFactory* sb_factory, InodeFactory* inode_factory);
	void Unregister(int type_id);
	int CreateSuperBlock(Session* session, int fs_type, SuperBlock** sbp); 
	int CreateSuperBlock(Session* session, const char* fs_type, SuperBlock** sbp); 
	int LoadSuperBlock(Session* session, dpo::common::ObjectId oid, int fs_type, SuperBlock** sbp); 
	int LoadSuperBlock(Session* session, dpo::common::ObjectId oid, const char* fs_type, SuperBlock** sbp); 
	int CreateInode(Session* session, Inode* parent, int inode_type, Inode** ipp);

private:
	int FSTypeStrToId(const char* fs_type);
	
	SuperBlockFactoryMap sb_factory_map_;
	FSTypeStrToIdMap     fstype_str_to_id_map_;
	InodeFactoryMap      inode_factory_map_;
};

} // namespace client

#endif // __STAMNOS_CLIENT_FILE_SYSTEM_OBJECT_MANAGER_H
