#include "pxfs/server/server.h"
#include "bcs/main/server/bcs.h"
#include "osd/main/server/osd.h"
#include "pxfs/mfs/server/mfs.h"
#include "pxfs/server/fs.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

        #include <sys/syscall.h>
        #define s_tid      syscall(SYS_gettid)

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
	char wd[128];
	getcwd(wd,128);
	printf("[%ld] Server::%s cur dir : %s", s_tid, __func__, wd);
	while (1) {
		sleep(1);
	}
}

} // namespace server
