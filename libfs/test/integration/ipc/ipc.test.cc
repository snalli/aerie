#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/integrationtest.h"
#include "pxfs/client/client_i.h"
#include "ipc.fixture.h"

using namespace client;

SUITE(IPC)
{
	TEST_FIXTURE(IPCFixture, TestServerIsAlive)
	{
		CHECK(Client::TestServerIsAlive()==0);
	}
}
