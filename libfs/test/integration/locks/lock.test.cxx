#include <stdio.h>
#include <stdlib.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/ut_barrier.h"
#include "client/client_i.h"
#include "client/lock_protocol.h"
#include "lock.fixture.hxx"

using namespace client;

static lock_protocol::LockId a = 1;
static lock_protocol::LockId b = 2;
static lock_protocol::LockId c = 3;

int
check_grant(CountRegion* region, lock_protocol::LockId lid)
{
	pthread_mutex_lock(&region->count_mutex);
	int x = lid & 0xff;
	if(region->ct[x] != 0) {
		fprintf(stderr, "error: server granted %016llx twice\n", lid);
		fprintf(stdout, "error: server granted %016llx twice\n", lid);
		return -1;
	}
	region->ct[x] += 1;
	pthread_mutex_unlock(&region->count_mutex);

	return 0;
}


int
check_release(CountRegion* region, lock_protocol::LockId lid)
{
	pthread_mutex_lock(&region->count_mutex);
	int x = lid & 0xff;
	if(region->ct[x] != 1) {
		fprintf(stderr, "error: client released un-held lock %016llx\n",  lid);
		return -1;
	}
	region->ct[x] -= 1;
	pthread_mutex_unlock(&region->count_mutex);
	return 0;
}


SUITE(Lock)
{
	TEST_FIXTURE(LockFixture, TestLockUnlockConcurrentClients1)
	{
		CHECK(Client::TestServerIsAlive() == 0);

		global_lckmgr->Acquire(a);
		CHECK(check_grant(region_, a) == 0);
		global_lckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
	}

	TEST_FIXTURE(LockFixture, TestLockUnlockConcurrentClients2)
	{
		CHECK(Client::TestServerIsAlive() == 0);

		for (int i=0; i<10; i++) {
			global_lckmgr->Acquire(a);
			CHECK(check_grant(region_, a) == 0);
			global_lckmgr->Release(a);
			CHECK(check_release(region_, a) == 0);
		}
	}

	TEST_THREAD_FIXTURE(LockFixture, TestLockUnlockConcurrentThreads, 2)
	{
		global_lckmgr->Acquire(a);
		CHECK(check_grant(TEST_THREAD_SHARED->region_, a) == 0);
		global_lckmgr->Release(a);
		CHECK(check_release(TEST_THREAD_SHARED->region_, a) == 0);
	}
}
