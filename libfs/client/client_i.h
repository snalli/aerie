// This is the master internal header file for the LibFS client library.

#ifndef _CLIENT_INTERNAL_H_ABG189
#define _CLIENT_INTERNAL_H_ABG189

#include "client/const.h"
#include "client/namespace.h"
#include "client/smgr.h"
#include "client/imgr.h"
#include "client/fdmgr.h"


namespace client {

extern NameSpace*             global_namespace;
extern StorageManager*        global_smgr;
extern FileDescriptorManager* fdmgr;

class Client {
public:
	static int Init(rpcc* rpc_client, int principal_id);
	static int Mount(const char* source, const char* target, const char* fstype, uint32_t flags);
	static int Mkfs(const char* target, const char* fstype, uint32_t flags);
	static int Open(const char* path, int flags, int mode);
	static int Close(int fd);
	static int Duplicate(int oldfd);
	static int Duplicate(int oldfd, int newfd);
	static int Mkdir(const char* path, int mode);
	static int Rmdir(const char* path);
};


} // namespace client

#endif /* _CLIENT_INTERNAL_H_ABG189 */
