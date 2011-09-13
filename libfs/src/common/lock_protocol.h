// lock protocol

#ifndef _LOCK_PROTOCOL_H_AKL156
#define _LOCK_PROTOCOL_H_AKL156

#include "rpc/rpc.h"


class lock_protocol {
public:
	class Mode;
	enum xxstatus { OK, RETRY, RPCERR, NOENT, IOERR };

	enum mode {
		NONE = -1,
		NL,   // not locked
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
		convert,
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


class lock_protocol::Mode {
public:
	enum mode {
		NONE = -1,
		NL,   // not locked
		SL,   // shared local
		SR,   // shared recursive
		IS,   // intent shared
		IX,   // intent exclusive
		XL,   // exclusive local
		XR,   // exclusive recursive
		IXSL, // intent exclusive and shared local
	};

	static bool         compatibility_table[][8];
	static int          severity_table[8];
	static std::string  mode2str_table[8];

	static std::string mode2str(int mode) {
		return mode2str_table[mode];
	}

	static int Severity(int mode) {
		return severity_table[mode];
	}
};

#endif /* _LOCK_PROTOCOL_H_AKL156 */
