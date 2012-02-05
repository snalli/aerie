#ifndef __STAMNOS_TEST_RPC_FIXTURE_H
#define __STAMNOS_TEST_RPC_FIXTURE_H

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "client/config.h"
#include "client/client_i.h"

using namespace client;

struct RPCFixture {
	static bool            initialized;
	static pthread_mutex_t mutex;

	RPCFixture() 
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
			Config::Init();
			Client::InitRPC(principal_id, xdst.c_str());
		}
		pthread_mutex_unlock(&mutex);
	}

	~RPCFixture() 
	{ 
	}
};


#endif // __STAMNOS_TEST_RPC_FIXTURE_H
