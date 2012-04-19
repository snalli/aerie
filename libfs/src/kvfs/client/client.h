// This is the master internal header file for the LibFS client library.

#ifndef __STAMNOS_KVFS_CLIENT_INTERNAL_H
#define __STAMNOS_KVFS_CLIENT_INTERNAL_H

#include "kvfs/client/const.h"
#include "kvfs/client/session.h"
#include "kvfs/client/table.h"

namespace client {

//class FileSystemObjectManager;  // forward declaration

//extern NameSpace*                  global_namespace;
extern Session*                    global_session;
extern Ipc*                        global_ipc_layer;        
extern osd::client::StorageSystem* global_storage_system;
//extern FileSystemObjectManager*    global_fsomgr;

class Client {
public:
	static int Init(const char* xdst);
	static int Init(int argc, char* argv[]);
	static int Shutdown(); 
	static Session* CurrentSession();
	static int Mount(const char* source, uint32_t flags);
	static int Put(const char* key, const char* src, uint64_t n);
	static int Get(const char* key, char* dst);
	static int Delete(const char* key);
	static int Sync();
	static int TestServerIsAlive();

private:
	static ::client::Table* tp_;
};


} // namespace client

#endif /* __STAMNOS_FS_CLIENT_INTERNAL_H */
