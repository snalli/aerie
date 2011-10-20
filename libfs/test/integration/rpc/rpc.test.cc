#include <stdio.h>
#include <stdlib.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "client/client_i.h"
#include "rpc.fixture.h"

using namespace client;

SUITE(RPC)
{
	TEST_FIXTURE(RPCFixture, TestServerIsAlive)
	{
		CHECK(Client::TestServerIsAlive()==0);
	}
}
