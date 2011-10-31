// This is the master internal header file for the LibFS client library.

#ifndef _CLIENT_INTERNAL_H_ABG189
#define _CLIENT_INTERNAL_H_ABG189

#include "client/const.h"
#include "client/namespace.h"
#include "client/smgr.h"
#include "client/imgr.h"
#include "client/file.h"


namespace client {

class LockManager;
class HLockManager;

extern NameSpace*        global_namespace;
extern StorageManager*   global_smgr;
extern FileManager*      global_fmgr;
extern InodeManager*     global_imgr;
extern LockManager*      global_lckmgr;
extern HLockManager*     global_hlckmgr;

extern rpcc*             rpc_client;
extern rpcs*             rpc_server;
extern std::string       id;


class Client {
public:
	static int InitRPC(int principal_id, const char* xdst);
	static int Init(int principal_id, const char* xdst);
	static int Shutdown(); 
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
	static int SetCurWrkDir(const char* path);
	static int GetCurWrkDir(const char* path, size_t size)
	static uint64_t Seek(int fd, uint64_t offset, int whence);
	static int TestServerIsAlive();
};


} // namespace client

#endif /* _CLIENT_INTERNAL_H_ABG189 */
