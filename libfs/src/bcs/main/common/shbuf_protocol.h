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


#endif // __STAMNOS_SHARED_BUFFER_PROTOCOL_H
