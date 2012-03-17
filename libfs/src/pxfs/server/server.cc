#include "pxfs/server/server.h"
#include "ipc/main/server/ipc.h"
#include "ssa/main/server/ssa.h"
#include "ssa/main/server/sessionmgr.h"
#include "pxfs/mfs/server/mfs.h"
#include "pxfs/server/fs.h"


namespace server {

Server* Server::instance_ = NULL;


// this is not thread safe. Must be called at least once while single-threaded 
// to ensure multiple threads won't race trying to construct the server instance
Server*
Server::Instance() 
{
	if (!instance_) {
		instance_ = new Server();
	}
	return instance_;
}


void 
Server::Start(const char* pathname, int flags, int port)
{
	port_ = port;

	ipc_layer_ = new ::server::Ipc(port);
	ipc_layer_->Init();

	assert(FileSystem::Load(ipc_layer_, pathname, flags, &fs_) == E_SUCCESS);

	sessionmgr_ = new SessionManager<Session>(ipc_layer_, fs_->storage_system());
	assert(sessionmgr_->Init() == E_SUCCESS);
}

} // namespace server
