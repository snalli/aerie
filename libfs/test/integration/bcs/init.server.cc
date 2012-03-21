#include "bcs/bcs.h"
#include "pxfs/server/server.h"
#include "test/integration/bcs/ipc.handlers.h"
#include "test/integration/bcs/shbuf.h"

int TestInit()
{
	int ret;
	if ((ret = IpcTestHandlers::Register(server::Server::Instance()->ipc_layer())) < 0) {
		return ret;
	}

	return E_SUCCESS;
}
