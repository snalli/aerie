#ifndef __STAMNOS_SHARED_BUFFER_PROTOCOL_H
#define __STAMNOS_SHARED_BUFFER_PROTOCOL_H

#include <string>
#include "bcs/rpcnum.h"
#include "bcs/main/common/shbuf.h"

class SharedBufferProtocol {
public:
	enum RpcNumbers {
		DEFINE_RPC_NUMBER(BCS_SHARED_BUFFER_PROTOCOL)
	};
};


inline marshall& operator<<(marshall &m, SharedBufferDescriptor& val) {
    m << val.path_;
    m << val.size_;
    return m;
}


inline unmarshall& operator>>(unmarshall &u, SharedBufferDescriptor& val) {
    u >> val.path_;
    u >> val.size_;
    return u;
}


#endif // __STAMNOS_SHARED_BUFFER_PROTOCOL_H
