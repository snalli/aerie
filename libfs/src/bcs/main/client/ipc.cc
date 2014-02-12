#include "bcs/main/client/ipc.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include "bcs/bcs.h"
#include "common/errno.h"
#include "bcs/main/common/ipc_protocol.h"
#include "bcs/main/client/shbuf.h"
#include <stdio.h>

namespace client {

Ipc::Ipc(const char* xdst)
	: xdst_(xdst)
{ }


int 
Ipc::Init()
{
	
	int                         r;
	struct sockaddr_in          dst; //server's ip address
	int                         rport;
	std::ostringstream          host;
	std::ostringstream          ss;
	const char*                 hname;
	IpcProtocol::SubscribeReply rep;
	std::string                 idstr;
	
//	printf("\nInitializing IPC Layer...");	

// setup RPC for making calls to the server
#ifdef _CLT2SVR_RPCNET
	rpcnet::make_sockaddr(xdst_.c_str(), &dst);
	rpcc_ = new rpcnet::rpcc(dst);
	assert (rpcc_->bind() == 0);
#endif
#ifdef _CLT2SVR_RPCFAST
	host.str("");
	host << "/tmp/server_rpcs_";
	host << xdst_;
	rpcc_ = new rpcfast::rpcc(host.str().c_str());
	assert (rpcc_->bind() == 0);
#endif


	// setup RPC for receiving callbacks from the server
#ifdef _SVR2CLT_RPCNET
	srandom(getpid());
	rport = 20000 + (getpid() % 10000);
	rpcs_ = new rpcnet::rpcs(rport);
	hname = "127.0.0.1";
	host.str("");
	host << hname << ":" << rport;
	idstr = host.str();
	//std::cout << "Client: id=" << idstr <<std::endl;
#endif
#ifdef _SVR2CLT_RPCFAST
	srandom(getpid());
	rport = 20000 + (getpid() % 10000);
	ss << "/tmp/client_rpcs_";
	ss << rport;
	rpcs_ = new rpcfast::rpcs(ss.str().c_str());
	rpcs_->main_service_loop();
	idstr = ss.str();
	//std::cout << "Client: id=" << idstr <<std::endl;
#endif

	// contact the server and tell him my rpc address to subscribe 
	// for async rpc response
	if ((r = rpcc_->call(IpcProtocol::kRpcSubscribe, rpcc_->id(), idstr, rep)) != 0) 
	{
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr), 
		        "failed to subscribe client: %u\n", rpcc_->id());
	}
//	printf("\nSUCCESS");
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
