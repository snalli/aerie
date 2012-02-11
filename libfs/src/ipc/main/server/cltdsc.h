#ifndef __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H
#define __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H

#include "ipc/backend/rpc.h"

namespace server {

class ClientDescriptor {
public:
	ClientDescriptor(int clt, rpcc* cl);
	rpcc* rpc() { return rpc_; }
	
protected:
	rpcc* rpc_;
	int   clt_;
};

} // namespace server

#endif // __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H
