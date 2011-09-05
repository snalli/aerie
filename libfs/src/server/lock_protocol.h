// lock protocol

#ifndef _SERVER_LOCK_PROTOCOL_H_AKL156
#define _SERVER_LOCK_PROTOCOL_H_AKL156

#include "rpc/rpc.h"

class lock_protocol {
public:
	enum xxstatus { OK, RETRY, RPCERR, NOENT, IOERR };
	typedef int status;
	typedef unsigned long long LockId;
	enum rpc_numbers {
		acquire_exclusive = 0x7001,
		acquire_shared = 0x7002,
		release,
		subscribe,	// for lab 5
		stat
	};
};


class rlock_protocol {
public:
	enum xxstatus { OK, RPCERR };
	typedef int status;
	enum rpc_numbers {
		revoke_release = 0x8001,
		revoke_downgrade = 0x8002,
		retry = 0x8003
	};
};

#endif /* _SERVER_LOCK_PROTOCOL_H_AKL156 */
