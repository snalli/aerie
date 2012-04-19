#include "bcs/main/server/ipc.h"
#include <sstream>
#include "bcs/main/common/ipc_protocol.h"
#include "bcs/main/server/sessionmgr.h"
#include "bcs/main/server/shbufmgr.h"
#include "bcs/main/common/rtconfig.h"
#include "bcs/main/common/debug.h"
#include "common/util.h"
#include "common/errno.h"

namespace server {

Ipc::Ipc(int port)
{
#ifdef _CLT2SVR_RPCNET
	pthread_mutex_init(&mutex_, NULL);
	rpcs_ = new rpcnet::rpcs(port);
	sessionmgr_ = new BaseSessionManager();
#endif
#ifdef _CLT2SVR_RPCFAST
	std::stringstream ss;

	pthread_mutex_init(&mutex_, NULL);
	ss << "/tmp/server_rpcs_";
	ss << port;
	rpcs_ = new rpcfast::rpcs(ss.str().c_str());
	sessionmgr_ = new BaseSessionManager();
	rpcs_->main_service_loop();
#endif
}


int
Ipc::Init()
{
	int   ret;
	
	rpcs_->reg(IpcProtocol::kRpcServerIsAlive, this, &Ipc::Alive);
	rpcs_->reg(IpcProtocol::kRpcSubscribe, this, &Ipc::Subscribe);
	if ((shbufmgr_ = new SharedBufferManager(this)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = shbufmgr_->Init()) < 0) {
		return ret;
	}
	return runtime_config_.Init();
}


Ipc::~Ipc()
{
	std::map<int, ClientDescriptor*>::iterator itr;

	pthread_mutex_lock(&mutex_);
	for (itr = clients_.begin(); itr != clients_.end(); ++itr) {
		delete itr->second;
	}
	pthread_mutex_unlock(&mutex_);
}


ClientDescriptor* 
Ipc::Client(int clt) 
{
	return clients_[clt];
}


int
Ipc::Subscribe(int clt, std::string id, IpcProtocol::SubscribeReply& rep)
{
#ifdef _SVR2CLT_RPCNET
	int                  ret;
	sockaddr_in          dstsock;
	rpcnet::rpcc*        rpccl;
	IpcProtocol::status  r = IpcProtocol::OK;

	pthread_mutex_lock(&mutex_);
	rpcnet::make_sockaddr(id.c_str(), &dstsock);
	rpccl = new rpcnet::rpcc(dstsock);
	ClientDescriptor* cl_dsc = new ClientDescriptor(clt, rpccl);
	if ((ret = cl_dsc->Init()) < 0) {
		return -ret;
	}
	if (rpccl->bind() == 0) {
		clients_[clt] = cl_dsc;
	} else {
		DBG_LOG(DBG_WARNING, DBG_MODULE(server_bcs), "failed to bind client %d\n", clt);
	}
	pthread_mutex_unlock(&mutex_);
	return r;
#endif 

#ifdef _SVR2CLT_RPCFAST
	int                  ret;
	rpcfast::rpcc*       rpccl;
	IpcProtocol::status  r = IpcProtocol::OK;

	pthread_mutex_lock(&mutex_);
	rpccl = new rpcfast::rpcc(id.c_str());
	ClientDescriptor* cl_dsc = new ClientDescriptor(clt, rpccl);
	if ((ret = cl_dsc->Init()) < 0) {
		return -ret;
	}
	if (rpccl->bind() == 0) {
		clients_[clt] = cl_dsc;
	} else {
		DBG_LOG(DBG_WARNING, DBG_MODULE(server_bcs), "failed to bind client %d\n", clt);
	}
	pthread_mutex_unlock(&mutex_);
	return r;
#endif 
}


int 
Ipc::Alive(const unsigned int principal_id, int& r)
{
	r = 0;

	return 0;
}


/*
 * RUNTIME CONFIGURATION 
 */


int
Ipc::RuntimeConfig::Init()
{
	return E_SUCCESS;
}


} // namespace server
