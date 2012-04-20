#ifndef __STAMNOS_IPC_PROTOCOL_H
#define __STAMNOS_IPC_PROTOCOL_H

#include <string>
#include "bcs/rpcnum.h"
#include "bcs/main/common/shbuf.h"

class IpcProtocol {
public:
	enum xxstatus { OK, RPCERR };
	typedef int status;
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(BCS_IPC_PROTOCOL)
	};

	class SubscribeReply {
	public:
	};
};

namespace rpcfast {

inline marshall& operator<<(marshall &m, IpcProtocol::SubscribeReply& val) {
	return m;
}


inline unmarshall& operator>>(unmarshall &u, IpcProtocol::SubscribeReply& val) {
	return u;
}

} // namespace rpcfast

namespace rpcnet {

inline marshall& operator<<(marshall &m, IpcProtocol::SubscribeReply& val) {
	return m;
}


inline unmarshall& operator>>(unmarshall &u, IpcProtocol::SubscribeReply& val) {
	return u;
}

} // namespace rpcnet

#endif // __STAMNOS_IPC_PROTOCOL_H
