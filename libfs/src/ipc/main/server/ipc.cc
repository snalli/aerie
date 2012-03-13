#include "ipc/main/server/ipc.h"
#include "ipc/main/common/ipc_protocol.h"
#include "ipc/main/server/sessionmgr.h"
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
	rpcs_->reg(IpcProtocol::kRpcServerIsAlive, this, &Ipc::Alive);
	rpcs_->reg(IpcProtocol::kRpcSubscribe, this, &Ipc::Subscribe);
	return E_SUCCESS;
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
	sockaddr_in          dstsock;
	rpcc*                rpccl;
	IpcProtocol::status  r = IpcProtocol::OK;

	pthread_mutex_lock(&mutex_);
	make_sockaddr(id.c_str(), &dstsock);
	rpccl = new rpcc(dstsock);
	ClientDescriptor* cl_dsc = new ClientDescriptor(clt, rpccl);
	if (rpccl->bind() == 0) {
		clients_[clt] = cl_dsc;
	} else {
		printf("failed to bind to client %d\n", clt);
	}
	pthread_mutex_unlock(&mutex_);
	return r;
}

/* TODO
int
Ipc::ClientToSession(int clt, IpcSession** session)
{
	return sessionmgr_->ClientToSession(clt, session);
}
*/

int 
Ipc::Alive(const unsigned int principal_id, int& r)
{
	r = 0;

	return 0;
}


} // namespace server
