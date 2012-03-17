#ifndef __STAMNOS_IPC_RPC_NUMBERS_H
#define __STAMNOS_IPC_RPC_NUMBERS_H

#define ENUM_NUMBER(module, protocol, handle)   \
    module##_##protocol##_##handle,

#define GET_RPC_NUMBER(module, protocol, handle)   \
    handle = module##_##protocol##_##handle,


// Module/protocol RPC numbers

#define SSA_LOCK_PROTOCOL(ACTION)              \
	ACTION(ssa, lock_protocol, acquire)

class RpcNumbers {
public:

	enum {
		null_rpc = 4000,
		SSA_LOCK_PROTOCOL(ENUM_NUMBER)
		
	};

};

enum rpc_numbers {
	SSA_LOCK_PROTOCOL(GET_RPC_NUMBER)
};


#endif // __STAMNOS_IPC_RPC_NUMBERS_H
