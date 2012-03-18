#ifndef __STAMNOS_TEST_IPC_HANDLERS_H
#define __STAMNOS_TEST_IPC_HANDLERS_H

#include "bcs/bcs.h"

class IpcTestHandlers {
public:

	static int Register(server::Ipc* ipc);

	int TestAdd(int a, int b, int& r);
	int TestEcho(std::string s, std::string& r);
};

#endif // __STAMNOS_TEST_IPC_HANDLERS_H
