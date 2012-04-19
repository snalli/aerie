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

	unsigned int id() { return rpcc_->id(); }

	static int Create(const char* xdst);
	int Test();

	RPC_REGISTER_HANDLER(rpcs_)
#ifdef _CLT2SVR_RPCNET	
	RPCNET_CALL(rpcc_, rpcnet::rpcc::to_max)
#endif
#ifdef _CLT2SVR_RPCFAST
	RPCFAST_CALL(rpcc_)
#endif

private:
	std::string                xdst_;
	/// the RPC object through which we make calls to the server
#ifdef _CLT2SVR_RPCNET	
	rpcnet::rpcc*              rpcc_;
#endif
#ifdef _CLT2SVR_RPCFAST	
	rpcfast::rpcc*             rpcc_;
#endif
	/// the RPC object through which we receive callbacks from the server
#ifdef _SVR2CLT_RPCNET	
	rpcnet::rpcs*              rpcs_;
#endif
#ifdef _SVR2CLT_RPCFAST	
	rpcfast::rpcs*             rpcs_;
#endif
};

} // namespace client

#endif // __STAMNOS_BCS_CLIENT_H
