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

	enum flag {
		FLG_NOQUE = 0x1,  // don't queue client if can't grant request
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
		CARDINALITY 
	};

	static std::string mode2str(int mode) {
		return mode2str_table[mode];
	}

	// mode1 less-than        mode2: returns 1
	// mode1 greater-than     mode2: returns -1
	// mode1 not-ordered-with mode2: returns 0
	static int PartialOrder(int mode1, int mode2) {
		if (mode1 == lock_protocol::IX && mode2 == lock_protocol::IXSL) {
			return -1;
		} else if (mode1 == lock_protocol::IXSL && mode2 == lock_protocol::IX) {
			return 1;
		} else {
			int s1 = severity_table[mode1];
			int s2 = severity_table[mode2];
			if (s1 > s2) {
				return 1;
			} else if (s1 < s2) {
				return -1;
			} 
		}
		return 0; // no ordering
	}

	static int Successor(int mode) {
		return successor_table[mode];
	}

	static bool         compatibility_table[][lock_protocol::Mode::CARDINALITY];

private:
	static std::string  mode2str_table[lock_protocol::Mode::CARDINALITY];
	static int          severity_table[lock_protocol::Mode::CARDINALITY];
	static int          successor_table[lock_protocol::Mode::CARDINALITY];
};

#endif /* _LOCK_PROTOCOL_H_AKL156 */
