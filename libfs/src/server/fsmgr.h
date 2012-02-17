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
#include "dpo/dpo-opaque.h"
#include "dpo/main/common/obj.h"
#include "ipc/main/common/ipc_handler.h"


namespace server {

class FileSystemFactory; // forward declaration
class Session;           // forward declaration

class FileSystemManager {
	typedef google::dense_hash_map<int, FileSystemFactory*> FileSystemFactoryMap;
	typedef google::dense_hash_map<std::string, int>        FSTypeStrToIdMap;

public:
	FileSystemManager(Ipc* ipc, dpo::server::Dpo* dpo);
	int Init();
	void Register(int type_id, const char* type_str, FileSystemFactory* fs_factory);
	void Register(FileSystemFactory* fs_factory);
	void Unregister(int type_id);
	int CreateFileSystem(Session* session, const char* target, int fs_type, unsigned int flags); 
	int CreateFileSystem(Session* session, const char* target, const char* fs_type, unsigned int flags); 
	int MountFileSystem(Session* session, const char* target, int fs_type, unsigned int flags, dpo::common::ObjectId* oid); 
	int MountFileSystem(Session* session, const char* target, const char* fs_type, unsigned int flags, dpo::common::ObjectId* oid); 

	class IpcHandlers {
	public:
		int Register(FileSystemManager* module);
		int CreateFileSystem(unsigned int clt, std::string target, std::string fs_type, unsigned int flags, int& r);
		int MountFileSystem(unsigned int clt, std::string target, std::string fs_type, unsigned int flags, dpo::common::ObjectId& r);

	private:
		FileSystemManager* module_;
	};

private:
	int FSTypeStrToId(const char* fs_type);
	
	FileSystemFactoryMap fs_factory_map_;
	FSTypeStrToIdMap     fstype_str_to_id_map_;
	Ipc*                 ipc_;
	dpo::server::Dpo*    dpo_;
	IpcHandlers          ipc_handlers_;
};

} // namespace server

#endif // __STAMNOS_PXFS_SERVER_FILE_SYSTEM_MANAGER_H
