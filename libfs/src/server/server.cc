#include "server/server.h"
#include "ipc/ipc.h"
#include "dpo/main/server/dpo.h"
#include "ipc/main/server/ipc.h"
#include "mfs/server/mfs.h"
#include "server/fsmgr.h"
#include "server/sessionmgr.h"


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
	
	dpo_layer_ = new ::dpo::server::Dpo(ipc_layer_);
	dpo_layer_->Init();

	fsmgr_ = new ::server::FileSystemManager(ipc_layer_, dpo_layer_);
	assert(fsmgr_->Init() == E_SUCCESS);

	//session_factory = new SessionFactory;

	//ipc_layer_->Register(session_factory);
	
	// register statically known file system backends
	mfs::server::RegisterBackend(fsmgr_);

	sessionmgr_ = new SessionManager(ipc_layer_, dpo_layer_);
	assert(sessionmgr_->Init() == E_SUCCESS);
}

} // namespace server
