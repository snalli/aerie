#ifndef __STAMNOS_BCS_PROTOCOL_H
#define __STAMNOS_BCS_PROTOCOL_H

#include <string>
#include "bcs/rpcnum.h"

class IpcProtocol {
public:
	enum xxstatus { OK, RPCERR };
	typedef int status;
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(BCS_PROTOCOL)
	};

	class SharedBufferDescriptor {
	public:
		std::string  path_;
		unsigned int size_;
	};
	
	class SubscribeReply {
	public:
		SharedBufferDescriptor shbuf_dsc_;
	};
};


inline marshall& operator<<(marshall &m, IpcProtocol::SubscribeReply& val) {
	m << val.shbuf_dsc_.path_;
	m << val.shbuf_dsc_.size_;
	return m;
}


inline unmarshall& operator>>(unmarshall &u, IpcProtocol::SubscribeReply& val) {
	u >> val.shbuf_dsc_.path_;
	u >> val.shbuf_dsc_.size_;
	return u;
}


#endif // __STAMNOS_BCS_PROTOCOL_H
