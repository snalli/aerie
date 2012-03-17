#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/integrationtest.h"
#include "pxfs/client/client_i.h"
#include "test/integration/ipc/test_protocol.h"
#include "ipc.fixture.h"

using namespace client;

SUITE(IPC)
{
	TEST_FIXTURE(IPCFixture, TestServerIsAlive)
	{
		CHECK(Client::TestServerIsAlive()==0);
	}
	
	TEST_FIXTURE(IPCFixture, TestEcho)
	{
		std::string rep;
		std::string big(10, 'x');
		CHECK(client::global_ipc_layer->call(IpcTestProtocol::kTestEcho, big, rep) == E_SUCCESS);
		CHECK(rep.size() == big.size());
	}	

	TEST_FIXTURE(IPCFixture, TestAdd)
	{
		int rep;

		CHECK(client::global_ipc_layer->call(IpcTestProtocol::kTestAdd, 1, 2, rep) == E_SUCCESS);
		CHECK(rep == 3);
	}

	TEST_FIXTURE(IPCFixture, TestSum)
	{
		int rep;
		int sum = 0;
		int correct_sum = 0;
		
		for (int i=0; i<1024;i++) {
			correct_sum += i;
		}

		for (int i=0; i<1024;i++) {
			CHECK(client::global_ipc_layer->call(IpcTestProtocol::kTestAdd, sum, i, rep) == E_SUCCESS);
			sum = rep;
		}
		CHECK(sum == correct_sum);
	}


}
