#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "client/client_i.h"
#include "common/lock_protocol.h"
#include "client/libfs.h"
#include "lock.fixture.h"
#include "checklock.h"

using namespace client;

static lock_protocol::LockId a = 1;
static lock_protocol::LockId b = 2;
static lock_protocol::LockId c = 3;


SUITE(Lock)
{
	TEST_FIXTURE(LockFixture, TestSharedLockUnlock)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);

		EVENT("E1");
		global_lckmgr->Acquire(a, lock_protocol::Mode::SL, 0, unused);
		CHECK(check_grant_s(region_, a) == 0);
		EVENT("E2");
		global_lckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
		EVENT("E3");
	}

	TEST_FIXTURE(LockFixture, TestSharedLockXLUnlockLockSLUnlock)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		
		EVENT("E1");
		global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		CHECK(check_grant_x(region_, a) == 0);
		EVENT("E2");
		global_lckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
		EVENT("E3");
		global_lckmgr->Acquire(a, lock_protocol::Mode::SL, 0, unused);
		CHECK(check_grant_s(region_, a) == 0);
		EVENT("E4");
		global_lckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
		EVENT("E5");
	}
}
