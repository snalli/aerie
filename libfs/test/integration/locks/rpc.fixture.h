#ifndef _RPC_FIXTURE_HXX_AGL119
#define _RPC_FIXTURE_HXX_AGL119

#include <pthread.h>
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/ut_barrier.h"
#include "client/libfs.h"
#include "client/config.h"
#include "client/client_i.h"

using namespace client;

struct RPCFixture {
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
		Config::Init();
		Client::InitRPC(principal_id, xdst.c_str());
	}

	~RPCFixture() 
	{ 
	}
};


#endif /* _RPC_FIXTURE_HXX_AGL119 */
