#include "bcs/main/server/ipc.h"
#include "bcs/main/common/bcs_protocol.h"
#include "bcs/main/server/sessionmgr.h"
#include "bcs/main/common/config.h"
#include "common/util.h"
#include "common/errno.h"

namespace server {

Ipc::Ipc(int port)
{
	pthread_mutex_init(&mutex_, NULL);
	rpcs_ = new rpcs(port);
	sessionmgr_ = new BaseSessionManager();
}


int
Ipc::Init()
{
	char*  csize;
	
	rpcs_->reg(IpcProtocol::kRpcServerIsAlive, this, &Ipc::Alive);
	rpcs_->reg(IpcProtocol::kRpcSubscribe, this, &Ipc::Subscribe);
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
Ipc::Subscribe(int clt, std::string id, int& unused)
{
	int                  ret;
	sockaddr_in          dstsock;
	rpcc*                rpccl;
	IpcProtocol::status  r = IpcProtocol::OK;

	pthread_mutex_lock(&mutex_);
	make_sockaddr(id.c_str(), &dstsock);
	rpccl = new rpcc(dstsock);
	ClientDescriptor* cl_dsc = new ClientDescriptor(clt, rpccl);
	if ((ret = cl_dsc->Init()) < 0) {
		return -ret;
	}
	if (rpccl->bind() == 0) {
		clients_[clt] = cl_dsc;
	} else {
		printf("failed to bind to client %d\n", clt);
	}
	pthread_mutex_unlock(&mutex_);
	return r;
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


size_t Ipc::RuntimeConfig::sharedbuffer_size;

int
Ipc::RuntimeConfig::Init()
{
	int   ret;
	char* csize;

	if ((ret = Config::Lookup("ipc.sharedbuffer.size", &csize)) < 0) {
		return ret;
	}
	sharedbuffer_size = StringToSize(csize);
	
	return E_SUCCESS;
}

} // namespace server
