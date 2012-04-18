#ifndef __STAMNOS_BCS_SERVER_CLIENTDESCRIPTOR_H
#define __STAMNOS_BCS_SERVER_CLIENTDESCRIPTOR_H

#include "bcs/backend/rpc.h"
#include "bcs/main/common/macros.h"
#include "bcs/main/server/shbuf.h"

namespace server {

struct ClientDescriptor {
public:
	ClientDescriptor(int clt, rpcc* rpccl)
		: rpcc_(rpccl),
		  clt_(clt)
	{ }
	
	int Init();
	int clt() { return clt_; }

#ifdef _RPCSOCKET
	RPC_CALL(rpcc_, rpcc::to_max)
#endif
#ifdef _RPCFAST
	RPC_CALL(rpcc_)
#endif

protected:
	rpcc*         rpcc_;
	int           clt_;
};


} // namespace server

#endif // __STAMNOS_BCS_SERVER_CLIENTDESCRIPTOR_H
