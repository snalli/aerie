#include "bcs/main/client/ipc.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include "bcs/bcs.h"
#include "common/errno.h"
#include "bcs/main/common/ipc_protocol.h"
#include "bcs/main/client/shbuf.h"

namespace client {

Ipc::Ipc(const char* xdst)
	: xdst_(xdst)
{ }

static void* service_loop(void* arg)
{
	rpcs* r = (rpcs*) arg;
	r->main_service_loop();
}

int 
Ipc::InitRpcFast()
{
	int                         r;
	int                         rport;
	const char*                 hname;
	IpcProtocol::SubscribeReply rep;
	std::string                 idstr;
	std::stringstream           ss;
	std::stringstream           host;
	
	// setup RPC for making calls to the server
	host << "/tmp/server_rpcs_";
	host << xdst_;
	rpcc_ = new rpcc(host.str().c_str());
	assert (rpcc_->bind() == 0);

	// setup RPC for receiving callbacks from the server
	srandom(getpid());
	rport = 20000 + (getpid() % 10000);
	ss << "/tmp/client_rpcs_";
	ss << rport;
	rpcs_ = new rpcs(ss.str().c_str());
	sleep(1);
	pthread_t thread;
	pthread_create(&thread, NULL, service_loop, (void*) rpcs_);
	sleep(1); // to ensure the main_service_loop starts
	idstr = ss.str();
	std::cout << "Client: id=" << idstr <<std::endl;

	// contact the server and tell him my rpc address to subscribe 
	// for async rpc response
	if ((r = rpcc_->call(IpcProtocol::kRpcSubscribe, rpcc_->id(), idstr, rep)) != 0) 
	{
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr), 
		        "failed to subscribe client: %u\n", rpcc_->id());
	}
		DBG_LOG(DBG_DEBUG, DBG_MODULE(client_lckmgr), 
		        "subscribed client: %u\n", rpcc_->id());
	return E_SUCCESS;
}


int 
Ipc::InitRpcSocket()
{
#ifdef _RPCSOCKET
	int                         r;
	struct sockaddr_in          dst; //server's ip address
	int                         rport;
	std::ostringstream          host;
	const char*                 hname;
	IpcProtocol::SubscribeReply rep;
	std::string                 idstr;
	
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
	if ((r = rpcc_->call(IpcProtocol::kRpcSubscribe, rpcc_->id(), idstr, rep)) != 0) 
	{
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr), 
		        "failed to subscribe client: %u\n", rpcc_->id());
	}
	return E_SUCCESS;
#endif
}


int
Ipc::Init()
{
#ifdef _RPCFAST
	return InitRpcFast();
#endif

#ifdef _RPCSOCKET
	return InitRpcSocket();
#endif
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
