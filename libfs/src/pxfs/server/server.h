#ifndef __STAMNOS_PXFS_SERVER_H
#define __STAMNOS_PXFS_SERVER_H

#include "ssa/ssa-opaque.h"
#include "ssa/main/server/sessionmgr.h"
#include "ipc/ipc-opaque.h"
#include "pxfs/server/session.h"

namespace server {

class FileSystem; // forward declaration

class Server {
public:
	static Server* Instance();
	void Start(const char* pathname, int flags, int port);

	SessionManager<Session>* session_manager() { return sessionmgr_; }
	Ipc* ipc_layer() { return ipc_layer_; }

private:
	int                          port_;
	Ipc*                         ipc_layer_;
	FileSystem*                  fs_;
	SessionManager<Session>*     sessionmgr_;
	static Server*               instance_;
};


} // namespace server

#endif // __STAMNOS_PXFS_SERVER_H
