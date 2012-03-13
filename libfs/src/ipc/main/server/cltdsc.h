#ifndef __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H
#define __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H

#include "ipc/backend/rpc.h"
#include "ipc/main/common/macros.h"

namespace server {


struct ClientDescriptor {
public:
	ClientDescriptor(int clt, rpcc* rpccl)
		: clt_(clt),
		  rpcc_(rpccl)
	{ }
	
	int clt() { return clt_; }

	RPC_CALL(rpcc_, rpcc::to_max)

protected:
	rpcc*      rpcc_;
	int        clt_;
};


} // namespace server

#endif // __STAMNOS_IPC_SERVER_CLIENTDESCRIPTOR_H
