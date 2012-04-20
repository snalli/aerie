#include "cfs/server/server.h"
#include "bcs/main/server/bcs.h"
#include "osd/main/server/osd.h"
#include "cfs/server/cfs.h"


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
Server::Init(const char* pathname, int flags, int port)
{
	port_ = port;

	ipc_layer_ = new ::server::Ipc(port);
	ipc_layer_->Init();

	assert(FileSystem::Load(ipc_layer_, pathname, flags, &fs_) == E_SUCCESS);
}

void 
Server::Start()
{
	while (1) {
		sleep(1);
	}
}

} // namespace server
