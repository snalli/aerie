#ifndef __STAMNOS_IPC_CLIENT_H
#define __STAMNOS_IPC_CLIENT_H

#include "ipc/backend/rpc.h"

namespace client {

class Ipc {
public:
	Ipc(const char* xdst);

	int Init();

	rpcc* cl2srv() { return cl2srv_; }
	rpcs* srv2cl() { return srv2cl_; }

	unsigned int id() { return cl2srv_->id(); }

	static int Create(const char* xdst);
	int Test();

private:
	std::string                xdst_;
	/// the RPC object through which we make calls to the server
	rpcc*                      cl2srv_;
	/// the RPC object through which we receive callbacks from the server
	rpcs*                      srv2cl_;
};

} // namespace client

#endif // __STAMNOS_IPC_CLIENT_H
