#ifndef __STAMNOS_BCS_CLIENT_H
#define __STAMNOS_BCS_CLIENT_H

#include "bcs/backend/rpc.h"
#include "bcs/main/common/macros.h"
#include "bcs/main/client/bcs-opaque.h"

namespace client {

class Ipc {
public:
	Ipc(const char* xdst);

	int Init();
	int InitRpcSocket();
	int InitRpcFast();

	unsigned int id() { return rpcc_->id(); }

	static int Create(const char* xdst);
	int Test();

	RPC_REGISTER_HANDLER(rpcs_)
#ifdef _RPCSOCKET	
	RPC_CALL(rpcc_, rpcc::to_max)
#endif
#ifdef _RPCFAST
	RPC_CALL(rpcc_)
#endif

private:
	std::string                xdst_;
	/// the RPC object through which we make calls to the server
	rpcc*                      rpcc_;
	/// the RPC object through which we receive callbacks from the server
	rpcs*                      rpcs_;
};

} // namespace client

#endif // __STAMNOS_BCS_CLIENT_H
