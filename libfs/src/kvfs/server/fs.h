/**
 * \file fs.h
 *
 * \brief File System
 *
 */

#ifndef __STAMNOS_KVFS_SERVER_FILE_SYSTEM_H
#define __STAMNOS_KVFS_SERVER_FILE_SYSTEM_H

#include <pthread.h>
#include <string>
#include <google/dense_hash_map>
#include "osd/osd-opaque.h"
#include "osd/main/common/obj.h"
#include "kvfs/server/session.h"
#include "kvfs/common/fs_protocol.h"


namespace server {


/**
 * Each server supports a single file system instance.  
 * We favor this for several reasons:
 * 1. don't need extra layer of indirection to support multiple file system 
 *    instances. 
 * 2. better reliability as a fault in one file system doesn't affect others 
 * 3) can use static polymorphism through C++ templates to modularize the 
 *     implementation of the storage system
 * Drawbacks:
 * 1) copy between two filesystems cannot be done in a single address space. 
 *    but it can be done over shared memory
 */

typedef osd::server::StorageSystemT<Session> StorageSystem;

class FileSystem {
public:
	static int Make(const char* target, size_t nblocks, size_t block_size, int flags);
	static int Load(Ipc* ipc, const char* source, unsigned int flags, FileSystem** fsp);

	FileSystem(Ipc* ipc, StorageSystem* storage_system);
	int Init();
	int Mount(int clt, const char* source, unsigned int flags, FileSystemProtocol::MountReply& rep); 

	StorageSystem* storage_system() { return storage_system_; }

	class IpcHandlers {
	public:
		int Register(FileSystem* module);
		int Mount(unsigned int clt, std::string source, unsigned int flags, FileSystemProtocol::MountReply& rep);

	private:
		FileSystem* module_;
	};

private:
	pthread_mutex_t     mutex_;
	Ipc*                ipc_;
	StorageSystem*      storage_system_;
	IpcHandlers         ipc_handlers_;
};

} // namespace server

#endif // __STAMNOS_KVFS_SERVER_FILE_SYSTEM_MANAGER_H
