#ifndef __STAMNOS_IPC_PROTOCOL_H
#define __STAMNOS_IPC_PROTOCOL_H

class IpcProtocol {
public:
	enum xxstatus { OK, RPCERR };
	typedef int status;
	enum RpcNumbers {
		kRpcServerIsAlive = 22,
		kRpcSubscribe,
	};

};

#endif // __STAMNOS_IPC_PROTOCOL_H
