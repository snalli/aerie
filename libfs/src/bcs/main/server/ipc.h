/**
 * \brief Interprocess communication substrate
 */
#ifndef __STAMNOS_BCS_SERVER_IPC_H
#define __STAMNOS_BCS_SERVER_IPC_H

#include <pthread.h>
#include "bcs/backend/rpc.h"
#include "bcs/main/common/macros.h"
#include "bcs/main/server/cltdsc.h"
#include "bcs/main/server/sessionmgr.h"
#include "bcs/main/server/bcs-opaque.h"
#include "bcs/main/common/ipc_protocol.h"

namespace server {


class Ipc {
public:

	Ipc(int port);
	~Ipc();

	int Init();
	rpcs* rpc() { return rpcs_; }
	
	ClientDescriptor* Client(int clt);

	int Subscribe(int clt, std::string id, IpcProtocol::SubscribeReply& rep);
	int Alive(const unsigned int principal_id, int& r);
	BaseSessionManager* session_manager() { return sessionmgr_; }
	SharedBufferManager* shbuf_manager() { return shbufmgr_; }
	RPC_REGISTER_HANDLER(rpcs_)

	class RuntimeConfig {
	public:
		static int Init();
	};

private:
	pthread_mutex_t                  mutex_;
	std::map<int, ClientDescriptor*> clients_;
	rpcs*                            rpcs_;
	int                              port_;
	BaseSessionManager*              sessionmgr_;
	SharedBufferManager*             shbufmgr_;
	RuntimeConfig                    runtime_config_;
};
	

} // namespace server

#endif // __STAMNOS_BCS_SERVER_IPC_H
