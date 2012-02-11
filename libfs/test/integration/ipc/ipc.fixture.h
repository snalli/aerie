#ifndef __STAMNOS_TEST_IPC_FIXTURE_H
#define __STAMNOS_TEST_IPC_FIXTURE_H

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "client/config.h"
#include "client/client_i.h"

using namespace client;

struct IPCFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;

	IPCFixture() 
	{
		std::string xdst;
		std::string principal_str;
		int         principal_id;

		CHECK(TESTFW->ArgVal("host", xdst) == 0);
		if (TESTFW->ArgVal("principal", principal_str) < 0) {
			principal_id = getuid();
		} else {
			principal_id = atoi(principal_str.c_str());
		}
		printf("IPCFIXTURE\n");
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			initialized = true;
			Config::Init();
			global_ipc_layer = new Ipc(xdst.c_str());
			global_ipc_layer->Init();
		}
		pthread_mutex_unlock(&mutex);
	}

	~IPCFixture() 
	{ 
	}
};


#endif // __STAMNOS_TEST_IPC_FIXTURE_H
