#ifndef __STAMNOS_IPC_SERVER_H
#define __STAMNOS_IPC_SERVER_H

#include <pthread.h>
#include "ipc/backend/rpc.h"
#include "ipc/main/common/macros.h"
#include "ipc/main/server/cltdsc.h"
#include "ipc/main/server/sessionmgr.h"

namespace server {


class Ipc {
public:

	Ipc(int port);
	~Ipc();

	int Init();
	rpcs* rpc() { return rpcs_; }
	
	ClientDescriptor* Client(int clt);

	int Subscribe(int clt, std::string id, int& unused);
	int Alive(const unsigned int principal_id, int& r);
	BaseSessionManager* session_manager() { return sessionmgr_; }

	RPC_REGISTER_HANDLER(rpcs_)

private:
	pthread_mutex_t                  mutex_;
	std::map<int, ClientDescriptor*> clients_;
	rpcs*                            rpcs_;
	int                              port_;
	BaseSessionManager*              sessionmgr_;
};


} // namespace server

#endif // __STAMNOS_IPC_SERVER_H