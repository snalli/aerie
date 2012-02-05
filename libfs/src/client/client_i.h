// This is the master internal header file for the LibFS client library.

#ifndef __STAMNOS_FS_CLIENT_INTERNAL_H
#define __STAMNOS_FS_CLIENT_INTERNAL_H

#include "client/const.h"
#include "client/namespace.h"
#include "client/smgr.h"

class Registry;
namespace client { class FileManager; } 
namespace dpo { namespace client { class ObjectManager; } }
namespace dpo { namespace cc { namespace client { class LockManager; } } }
namespace dpo { namespace cc { namespace client { class HLockManager; } } }

namespace client {

extern NameSpace*                         global_namespace;
extern StorageManager*                    global_smgr;
extern FileManager*                       global_fmgr;
extern dpo::cc::client::LockManager*      global_lckmgr;
extern dpo::cc::client::HLockManager*     global_hlckmgr;
extern dpo::client::ObjectManager*        global_omgr;
extern Session*                           global_session;
extern Registry*                          global_registry;

extern rpcc*             rpc_client;
extern rpcs*             rpc_server;
extern std::string       id;


class Client {
public:
	static int InitRPC(int principal_id, const char* xdst);
	static int Init(int principal_id, const char* xdst);
	static int Shutdown(); 
	static Session* CurrentSession();
	static int Mount(const char* source, const char* target, const char* fstype, uint32_t flags);
	static int Mkfs(const char* target, const char* fstype, uint32_t flags);
	static int Open(const char* path, int flags, int mode);
	static int Close(int fd);
	static int Duplicate(int oldfd);
	static int Duplicate(int oldfd, int newfd);
	static int Write(int fd, const char* src, uint64_t n);
	static int Read(int fd, char* dst, uint64_t n);
	static int CreateDir(const char* path, int mode);
	static int DeleteDir(const char* path);
	static int Rename(const char* oldpath, const char* newpath);
	static int Link(const char* oldpath, const char* newpath);
	static int Unlink(const char* path);
	static int SetCurWrkDir(const char* path);
	static int GetCurWrkDir(const char* path, size_t size);
	static uint64_t Seek(int fd, uint64_t offset, int whence);
	static int TestServerIsAlive();
};


} // namespace client

#endif /* __STAMNOS_FS_CLIENT_INTERNAL_H */
