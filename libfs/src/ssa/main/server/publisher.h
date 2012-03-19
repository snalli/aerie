#ifndef __STAMNOS_SSA_SERVER_PUBLISHER_H
#define __STAMNOS_SSA_SERVER_PUBLISHER_H

#include "bcs/bcs.h"
#include "ssa/main/server/ssa-opaque.h"

namespace ssa {
namespace server {

class Publisher {
public:
	Publisher(::server::Ipc* ipc);

	int Init();
	int Publish(SsaSession* session);

	class IpcHandlers {
	public:
		int Register(Publisher* publisher);

		int Publish(int clt, int& r);

	private:
		Publisher* publisher_;
	}; 

private:
	::server::Ipc*                                 ipc_;
	Publisher::IpcHandlers                         ipc_handlers_;

};


} // namespace server
} // namespace ssa


#endif // __STAMNOS_SSA_SERVER_PUBLISHER_H
