#ifndef __STAMNOS_IPC_SERVER_H
#define __STAMNOS_IPC_SERVER_H

#include <pthread.h>
#include "ipc/backend/rpc.h"
#include "ipc/main/server/cltdsc.h"

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

private:
	pthread_mutex_t                  mutex_;
	std::map<int, ClientDescriptor*> clients_;
	rpcs*                            rpcs_;
	int                              port_;
};

} // namespace server

#endif // __STAMNOS_IPC_SERVER_H
