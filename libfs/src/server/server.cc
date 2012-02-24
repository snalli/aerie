#include "server/server.h"
#include "ipc/ipc.h"
#include "dpo/main/server/dpo.h"
#include "ipc/main/server/ipc.h"
#include "mfs/server/mfs.h"
#include "server/fsmgr.h"


namespace server {

Server* Server::instance_ = NULL;

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
	fsmgr_->Init();
	
	// register statically known file system backends
	mfs::server::RegisterBackend(fsmgr_);
}

} // namespace server
