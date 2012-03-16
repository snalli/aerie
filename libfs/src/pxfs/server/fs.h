/**
 * \file fs.h
 *
 * \brief File System
 *
 */

#ifndef __STAMNOS_PXFS_SERVER_FILE_SYSTEM_H
#define __STAMNOS_PXFS_SERVER_FILE_SYSTEM_H

#include <pthread.h>
#include <string>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "ssa/ssa-opaque.h"
#include "ssa/main/common/obj.h"
#include "ipc/main/common/ipc_handler.h"


namespace server {

class Session;           // forward declaration


/**
 * Each server supports a single file system instance.  
 * We favor this for several reasons:
 * 1. don't need extra layer of indirection to support multiple file system instances. 
 * 2. better reliability as a fault in one file system doesn't affect others 
 */
class FileSystem {
public:
	FileSystem(Ipc* ipc, ssa::server::StorageSystem* storage_system);
	int Init();
	int Create(const char* target, size_t nblocks, size_t block_size, int flags); 
	int Mount(int clt, const char* source, const char* target, unsigned int flags, ssa::common::ObjectId* oid); 

	class IpcHandlers {
	public:
		int Register(FileSystem* module);
		int Create(unsigned int clt, std::string target, unsigned int nblocks, unsigned int block_size, unsigned int flags, ssa::common::ObjectId& r);
		int Mount(unsigned int clt, std::string source, std::string target, unsigned int flags, ssa::common::ObjectId& r);

	private:
		FileSystem* module_;
	};

private:
	pthread_mutex_t              mutex_;
	ssa::server::StorageSystem*  storage_system_;
	Ipc*                         ipc_;
	IpcHandlers                  ipc_handlers_;
};

} // namespace server

#endif // __STAMNOS_PXFS_SERVER_FILE_SYSTEM_MANAGER_H
