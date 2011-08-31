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
		acquire = 0x7001,
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
		revoke = 0x8001,
		retry = 0x8002
	};
};

#endif /* _SERVER_LOCK_PROTOCOL_H_AKL156 */
