/**
 * \file fs.h
 *
 * \brief File System
 *
 */

#ifndef __STAMNOS_CFS_SERVER_FILE_SYSTEM_H
#define __STAMNOS_CFS_SERVER_FILE_SYSTEM_H

#include <pthread.h>
#include <string>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "osd/osd-opaque.h"
#include "osd/main/common/obj.h"
#include "cfs/server/session.h"
#include "cfs/common/fs_protocol.h"
#include "cfs/server/sb.h"


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
	int MakeFile(Session* session, const char* path, unsigned int flags, unsigned int mode, FileSystemProtocol::InodeNumber& ino);
	int MakeDir(Session* session, const char* path, unsigned int mode, FileSystemProtocol::InodeNumber& ino);
	int Unlink(Session* session, const char* path);
	int Read(Session* session, FileSystemProtocol::InodeNumber ino, void* buf, int count, int offset, int& n);
	int Write(Session* session, FileSystemProtocol::InodeNumber ino, void* buf, int count, int offset, int& n);
	int Link(Session* session, const char* oldpath, const char* newpath);
	int Namei(Session* session, const char* path, FileSystemProtocol::InodeNumber& ino);
	
	int Mount(int clt, const char* source, const char* target, unsigned int flags, FileSystemProtocol::MountReply& rep); 

	StorageSystem* storage_system() { return storage_system_; }

	class IpcHandlers {
	public:
		int Register(FileSystem* module);
		int Mount(unsigned int clt, std::string source, std::string target, unsigned int flags, FileSystemProtocol::MountReply& rep);
		int MakeFile(unsigned int clt, std::string path, unsigned int flags, unsigned int mode, FileSystemProtocol::InodeNumber& ino);
		int MakeDir(unsigned int clt, std::string path, unsigned int mode, FileSystemProtocol::InodeNumber& ino);
		int Read(unsigned int clt, FileSystemProtocol::InodeNumber ino, int count, int offset, int& r);
		int Write(unsigned int clt, FileSystemProtocol::InodeNumber ino, int count, int offset, int& r);
		int Link(unsigned int clt, std::string oldpath, std::string newpath, int& r);
		int Unlink(unsigned int clt, std::string path, int& r);
		int Namei(unsigned int clt, std::string path, FileSystemProtocol::InodeNumber& ino);

	private:
		FileSystem* module_;
	};

private:
	pthread_mutex_t     mutex_;
	Ipc*                ipc_;
	StorageSystem*      storage_system_;
	IpcHandlers         ipc_handlers_;
	SuperBlock*         sb_;
};

} // namespace server

#endif // __STAMNOS_CFS_SERVER_FILE_SYSTEM_MANAGER_H
