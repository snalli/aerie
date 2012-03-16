#include "pxfs/server/server.h"
#include "ipc/main/server/ipc.h"
#include "ssa/main/server/ssa.h"
#include "pxfs/mfs/server/mfs.h"
#include "pxfs/server/fs.h"
#include "pxfs/server/sessionmgr.h"


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
Server::Start(int port)
{
	port_ = port;

	ipc_layer_ = new ::server::Ipc(port);
	ipc_layer_->Init();

/*
	ssa_layer_ = new ::ssa::server::Dpo(ipc_layer_);
	ssa_layer_->Init();

	fsmgr_ = new ::server::FileSystemManager(ipc_layer_, ssa_layer_);
	assert(fsmgr_->Init() == E_SUCCESS);
*/

/*
	sessionmgr_ = new SessionManager(ipc_layer_, ssa_layer_);
	assert(sessionmgr_->Init() == E_SUCCESS);
*/
}

} // namespace server
