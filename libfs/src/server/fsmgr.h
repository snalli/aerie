/**
 * \file fsmgr.h
 *
 * \brief File System manager
 *
 */

#ifndef __STAMNOS_PXFS_SERVER_FILE_SYSTEM_MANAGER_H
#define __STAMNOS_PXFS_SERVER_FILE_SYSTEM_MANAGER_H

#include <string>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>

namespace server {

class FileSystemFactory; // forward declaration
class Session;           // forward declaration

class FileSystemManager {
	typedef google::dense_hash_map<int, FileSystemFactory*> FileSystemFactoryMap;
	typedef google::dense_hash_map<std::string, int>        FSTypeStrToIdMap;

public:
	FileSystemManager();
	void Register(int type_id, const char* type_str, FileSystemFactory* fs_factory);
	void Register(FileSystemFactory* fs_factory);
	void Unregister(int type_id);
	int CreateFileSystem(Session* session, const char* target, int fs_type); 
	int CreateFileSystem(Session* session, const char* target, const char* fs_type); 

	int CreateFileSystem(int clt, const char* target, const char* fs_type, int& r);
private:
	int FSTypeStrToId(const char* fs_type);
	
	FileSystemFactoryMap fs_factory_map_;
	FSTypeStrToIdMap     fstype_str_to_id_map_;
};

} // namespace server

#endif // __STAMNOS_PXFS_SERVER_FILE_SYSTEM_MANAGER_H
