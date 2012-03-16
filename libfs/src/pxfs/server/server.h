#ifndef __STAMNOS_PXFS_SERVER_H
#define __STAMNOS_PXFS_SERVER_H

#include "ssa/ssa-opaque.h"
#include "ipc/ipc-opaque.h"

namespace server {

class FileSystem; // forward declaration
class SessionManager; // forward declaration

class Server {
public:
	static Server* Instance();
	void Start(int port);

	SessionManager* session_manager() { return sessionmgr_; }

private:
	int                          port_;
	Ipc*                         ipc_layer_;
	FileSystem*                  fs_;
	SessionManager*              sessionmgr_;
	static Server*               instance_;
};


} // namespace server

#endif // __STAMNOS_PXFS_SERVER_H
