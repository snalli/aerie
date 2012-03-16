#ifndef __STAMNOS_PXFS_SERVER_H
#define __STAMNOS_PXFS_SERVER_H

#include "ssa/ssa-opaque.h"
#include "ipc/ipc-opaque.h"

class ChunkServer;       // forward declaration

namespace server {

class FileSystemManager; // forward declaration
class SessionManager; // forward declaration

class Server {
public:
	static Server* Instance();
	void Start(int port);

	SessionManager* session_manager() { return sessionmgr_; }

private:
	int                port_;
	ChunkServer*       chunk_server_;
	ssa::server::Dpo*  ssa_layer_;
	Ipc*               ipc_layer_;
	FileSystemManager* fsmgr_;
	SessionManager*    sessionmgr_;
	static Server*     instance_;
};


} // namespace server

#endif // __STAMNOS_PXFS_SERVER_H
