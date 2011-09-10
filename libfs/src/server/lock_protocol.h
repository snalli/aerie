// lock protocol

#ifndef _SERVER_LOCK_PROTOCOL_H_AKL156
#define _SERVER_LOCK_PROTOCOL_H_AKL156

#include "rpc/rpc.h"

class lock_protocol {
public:
	enum xxstatus { OK, TRY_UPGRADE, RETRY, RPCERR, NOENT, IOERR };
	enum mode {
		NONE = -1,
		FREE, // free
		SL,   // shared local
		SR,   // shared recursive
		IS,   // intent shared
		IX,   // intent exclusive
		XL,   // exclusive local
		XR,   // exclusive recursive
		IXSL, // intent exclusive and shared local
	};
	enum revoke {
		RVK_NO = 0,      // no revoke
		RVK_NL,      
		RVK_XL2SL,
		RVK_SR2SL,
		RVK_XR2XL,
		RVK_IXSL2IX
	};

	typedef int status;
	typedef unsigned long long LockId;
	enum rpc_numbers {
		acquire = 0x7001,
		release,
		downgrade,
		subscribe,
		stat
	};
};


class rlock_protocol {
public:
	enum xxstatus { OK, RPCERR };
	typedef int status;
	enum rpc_numbers {
		revoke = 0x8001,
		retry = 0x8002
	};
};

#endif /* _SERVER_LOCK_PROTOCOL_H_AKL156 */
