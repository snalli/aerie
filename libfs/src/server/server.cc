#include "server/server.h"
#include "ipc/ipc.h"
#include "dpo/main/server/dpo.h"
#include "ipc/main/server/ipc.h"
#include "chunkstore/chunkserver.h"
#include "mfs/server/mfs.h"
#include "server/fsmgr.h"

//FIXME: chunk_server and register_handlers should be modularized under corresponding layers
::ChunkServer* chunk_server;
void register_handlers(rpcs* serverp);


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
	chunk_server_ = new ::ChunkServer();
	chunk_server_->Init();
	chunk_server = chunk_server_;

	ipc_layer_ = new ::server::Ipc(port);
	ipc_layer_->Init();
	register_handlers(ipc_layer_->rpc());
	
	dpo_layer_ = new ::dpo::server::Dpo(ipc_layer_);
	dpo_layer_->Init();

	fsmgr_ = new ::server::FileSystemManager(ipc_layer_, dpo_layer_);
	fsmgr_->Init();
	
	// register statically known file system backends
	mfs::server::RegisterBackend(fsmgr_);
}

} // namespace server
