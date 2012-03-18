#include "test/integration/bcs/ipc.handlers.h"
#include "common/errno.h"
#include "bcs/bcs.h"
#include "pxfs/server/server.h"
#include "test/integration/bcs/test_protocol.h"


int 
IpcTestHandlers::Register(server::Ipc* ipc)
{
	IpcTestHandlers* test_handlers = new IpcTestHandlers();
	
	ipc->reg(::IpcTestProtocol::kTestAdd, test_handlers, 
	         &IpcTestHandlers::TestAdd);
	ipc->reg(::IpcTestProtocol::kTestEcho, test_handlers, 
	         &IpcTestHandlers::TestEcho);

	return E_SUCCESS;
}


int
IpcTestHandlers::TestAdd(int a, int b, int& r)
{
	//dbg_log(DBG_INFO, "Add: %d %d\n", a, b);
	
	r = a + b;
	return E_SUCCESS;
}


int 
IpcTestHandlers::TestEcho(std::string s, std::string& r)
{
	dbg_log(DBG_INFO, "Echo: string size=%d\n", s.size());

	r = s;
	return E_SUCCESS;
}
