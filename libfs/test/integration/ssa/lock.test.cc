#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "ssa/main/common/lock_protocol.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "lock.fixture.h"
#include "checklock.h"

using namespace client;

static lock_protocol::LockId a = 1;
static lock_protocol::LockId b = 2;
static lock_protocol::LockId c = 3;


SUITE(Lock)
{
	TEST_FIXTURE(LockFixture, TestLockSL)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		EVENT("E1");
		global_ssa_layer->lckmgr()->Acquire(a, lock_protocol::Mode::SL, 0, unused);
		EVENT("E2");
	}
	
	TEST_FIXTURE(LockFixture, TestLockXLConvertSL)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);

		EVENT("E1");
		global_ssa_layer->lckmgr()->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		global_ssa_layer->lckmgr()->Convert(a, lock_protocol::Mode::SL);
		EVENT("E3");
	}

	TEST_FIXTURE(LockFixture, TestLockXLConvertSLsynchronous)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);

		EVENT("E1");
		global_ssa_layer->lckmgr()->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		global_ssa_layer->lckmgr()->Convert(a, lock_protocol::Mode::SL, true);
		EVENT("E3");
	}

	TEST_FIXTURE(LockFixture, TestLockUnlock)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		EVENT("E1");
		global_ssa_layer->lckmgr()->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		CHECK(check_grant_x(region_, a) == 0);
		global_ssa_layer->lckmgr()->Release(a);
		CHECK(check_release(region_, a) == 0);
		EVENT("E3");
	}
	
	TEST_FIXTURE(LockFixture, TestLockUnlockTwoTimes)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		EVENT("E1");
		global_ssa_layer->lckmgr()->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		CHECK(check_grant_x(region_, a) == 0);
		global_ssa_layer->lckmgr()->Release(a);
		CHECK(check_release(region_, a) == 0);
		global_ssa_layer->lckmgr()->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		CHECK(check_grant_x(region_, a) == 0);
		EVENT("E3");
		usleep(1000); // give enough time for a competing thread to try to acquire the lock
		              // we would like to be able to express this in our schedule
		global_ssa_layer->lckmgr()->Release(a);
		CHECK(check_release(region_, a) == 0);
		EVENT("E4");
	}
	
	TEST_FIXTURE(LockFixture, TestLockUnlockMultipleTimes)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		EVENT("E1");
		for (int i=0; i<10; i++) {
			EVENT("E2");
			global_ssa_layer->lckmgr()->Acquire(a, lock_protocol::Mode::XL, 0, unused);
			CHECK(check_grant_x(region_, a) == 0);
			EVENT("E3");
			global_ssa_layer->lckmgr()->Release(a);
			CHECK(check_release(region_, a) == 0);
			EVENT("E4");
		}
	}


	TEST_FIXTURE(LockFixture, TestLockBUnlockB)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		global_ssa_layer->lckmgr()->Acquire(b, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		EVENT("E3");
		EVENT("E4");
		EVENT("E5");
		global_ssa_layer->lckmgr()->Release(b);
		EVENT("E6");
	}


	TEST_FIXTURE(LockFixture, TestLockALockB)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		global_ssa_layer->lckmgr()->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		EVENT("E3");
		global_ssa_layer->lckmgr()->Acquire(b, lock_protocol::Mode::XL, 0, unused);
		EVENT("E4");
	}

	TEST_FIXTURE(LockFixture, TestLockBLockA)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		global_ssa_layer->lckmgr()->Acquire(b, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		global_ssa_layer->lckmgr()->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E3");
	}
	
	TEST_FIXTURE(LockFixture, TestCancelLock)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		usleep(100000); // give enough time for a competing thread to acquire the lock
		              	// we would like to be able to express this in our schedule
		EVENT("E2");
		global_ssa_layer->lckmgr()->Cancel(b);
		EVENT("E3");
	}
}
