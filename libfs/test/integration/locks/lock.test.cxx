#include <stdio.h>
#include <stdlib.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "client/client_i.h"

using namespace client;

SUITE(Lock)
{
	TEST(TestLockUnlock)
	{
		CHECK(Client::TestServerIsAlive()==0);
		global_lckmgr->Acquire(1000);
		global_lckmgr->Release(1000);
	}
}
