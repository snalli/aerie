#include "ipc/main/client/ipc.h"
#include "common/debug.h"
#include "common/errno.h"
#include "ipc/main/common/ipc_protocol.h"

namespace client {

Ipc::Ipc(const char* xdst)
	: xdst_(xdst)
{ }


int 
Ipc::Init()
{
	int                r;
	struct sockaddr_in dst; //server's ip address
	int                rport;
	std::ostringstream host;
	const char*        hname;
	int                unused;
	std::string        idstr;
	int                principal_id = getuid();
	
	// setup RPC for making calls to the server
	make_sockaddr(xdst_.c_str(), &dst);
	rpcc_ = new rpcc(dst);
	assert (rpcc_->bind() == 0);

	// setup RPC for receiving callbacks from the server
	srandom(getpid());
	rport = 20000 + (getpid() % 10000);
	rpcs_ = new rpcs(rport);
	hname = "127.0.0.1";
	host << hname << ":" << rport;
	idstr = host.str();
	std::cout << "Client: id=" << idstr <<std::endl;

	// contact the server and tell him my rpc address to subscribe 
	// for async rpc response
	if ((r = rpcc_->call(IpcProtocol::kRpcSubscribe, rpcc_->id(), idstr, unused)) !=
	    0) 
	{
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr), 
		        "failed to subscribe client: %u\n", rpcc_->id());
	}
	return E_SUCCESS;
}


// factory method
int
Ipc::Create(const char* xdst) 
{
	Ipc* ipc;

	if ((ipc = new Ipc(xdst)) == NULL) {
		return -E_NOMEM;
	}
	return ipc->Init();
}


int
Ipc::Test()
{
	int r;
	rpcc_->call(IpcProtocol::kRpcServerIsAlive, 0, r);
	return r;
}

} // namespace client
