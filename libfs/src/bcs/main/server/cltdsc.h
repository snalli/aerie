#ifndef __STAMNOS_BCS_SERVER_CLIENTDESCRIPTOR_H
#define __STAMNOS_BCS_SERVER_CLIENTDESCRIPTOR_H

#include "bcs/backend/rpc.h"
#include "bcs/main/common/macros.h"
#include "bcs/main/server/shbuf.h"

namespace server {

#ifdef _SVR2CLT_RPCNET
struct ClientDescriptor {
public:
	ClientDescriptor(int clt, rpcnet::rpcc* rpccl)
		: rpcc_(rpccl),
		  clt_(clt)
	{ }
	
	int Init();
	int clt() { return clt_; }

	RPCNET_CALL(rpcc_, rpcnet::rpcc::to_max)

protected:
	rpcnet::rpcc* rpcc_;
	int           clt_;
};
#endif


#ifdef _SVR2CLT_RPCFAST
struct ClientDescriptor {
public:
	ClientDescriptor(int clt, rpcc* rpccl)
		: rpcc_(rpccl),
		  clt_(clt)
	{ }
	
	int Init();
	int clt() { return clt_; }

	RPCFAST_CALL(rpcc_)

protected:
	rpcc*         rpcc_;
	int           clt_;
};
#endif



} // namespace server

#endif // __STAMNOS_BCS_SERVER_CLIENTDESCRIPTOR_H
