#ifndef __STAMNOS_TEST_IPC_FIXTURE_H
#define __STAMNOS_TEST_IPC_FIXTURE_H

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "bcs/bcs.h"
#include "pxfs/client/client_i.h"

extern client::Ipc* client::global_ipc_layer;

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
		pthread_mutex_lock(&mutex);
		if (!initialized) {
			initialized = true;
			client::global_ipc_layer = new client::Ipc(xdst.c_str());
			client::global_ipc_layer->Init();
		}
		pthread_mutex_unlock(&mutex);
	}

	~IPCFixture() 
	{ 
	}
};


#endif // __STAMNOS_TEST_IPC_FIXTURE_H
