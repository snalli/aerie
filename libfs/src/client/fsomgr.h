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
 * therefore seen as an abstract factory as well but its interface
 * it's not exactly an AFT so we avoid the use of this term.
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

#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "rpc/rpc.h"


namespace client {

class SuperBlockFactory; // forward declaration
class InodeFactory;      // forward declaration


class FileSystemObjectManager {
	typedef google::dense_hash_map<int, SuperBlockFactory*> SuperBlockFactoryMap;
public:
	FileSystemObjectManager(rpcc* rpc_client, unsigned int principal_id);
	void Register(int magic, SuperBlockFactory* sb_factory, InodeFactory* inode_factory);
	void Unregister(int magic);

private:
	SuperBlockFactoryMap sb_factory_map_;
	//InodeFactoryMap*      inode_factory_map_;
	//InodeFactory*      inode_factory_;
};

} // namespace client

#endif // __STAMNOS_CLIENT_FILE_SYSTEM_OBJECT_MANAGER_H
