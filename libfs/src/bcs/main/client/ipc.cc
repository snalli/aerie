#include "bcs/main/client/ipc.h"
#include "bcs/bcs.h"
#include "bcs/main/client/shbuf.h"
#include "bcs/main/common/ipc_protocol.h"
#include "common/errno.h"
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <sys/socket.h>

namespace client
{

Ipc::Ipc(const char* xdst) : xdst_(xdst)
{
}

int Ipc::Init()
{

    int r;
    int rport;
    std::ostringstream host;
    std::ostringstream ss;
    const char* hname;
    IpcProtocol::SubscribeReply rep;
    std::string idstr;


// setup RPC for making calls to the server
#ifdef _CLT2SVR_RPCNET
    struct sockaddr_in dst; // server's ip address
    rpcnet::make_sockaddr(xdst_.c_str(), &dst);
    rpcc_ = new rpcnet::rpcc(dst);
    assert(rpcc_->bind() == 0);
#endif
#ifdef _CLT2SVR_RPCFAST
    host.str("");
    host << "/tmp/server_rpcs_";
    host << xdst_;
    rpcc_ = new rpcfast::rpcc(host.str().c_str());
    assert(rpcc_->bind() == 0);
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
#endif
#ifdef _SVR2CLT_RPCFAST
    srandom(getpid());
    rport = 20000 + (getpid() % 10000);
    ss << "/tmp/client_rpcs_";
    ss << rport;
    rpcs_ = new rpcfast::rpcs(ss.str().c_str());
    rpcs_->main_service_loop();
    idstr = ss.str();
#endif

    // contact the server and tell him my rpc address to subscribe
    if ((r = rpcc_->call(IpcProtocol::kRpcSubscribe, rpcc_->id(), idstr, rep)) != 0)
    {
        DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr), "failed to subscribe client: %u\n",
                rpcc_->id());
    }
    return E_SUCCESS;
}

// factory method
int Ipc::Create(const char* xdst)
{
    Ipc* ipc;

    if ((ipc = new Ipc(xdst)) == NULL)
    {
        return -E_NOMEM;
    }
    return ipc->Init();
}

int Ipc::Test()
{
    int r;
    rpcc_->call(IpcProtocol::kRpcServerIsAlive, 0, r);
    return r;
}

} // namespace client
