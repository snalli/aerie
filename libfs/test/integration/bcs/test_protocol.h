#ifndef __STAMNOS_TEST_IPC_PROTOCOL_H
#define __STAMNOS_TEST_IPC_PROTOCOL_H

#include "bcs/bcs.h"

class IpcTestProtocol {
public:
	
	enum RpcNumbers {
		kTestAdd = 4900,
		kTestEcho = 4901
	};
};


#endif // __STAMNOS_TEST_IPC_PROTOCOL_H
