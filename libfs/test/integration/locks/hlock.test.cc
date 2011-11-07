#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "common/lock_protocol.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "client/hlckmgr.h"
#include "hlock.fixture.h"
#include "checklock.h"

using namespace client;

static lock_protocol::LockId root = 1;
static lock_protocol::LockId a    = 2;
static lock_protocol::LockId b    = 3;
static lock_protocol::LockId c    = 4;


SUITE(HLock)
{
	TEST_FIXTURE(HLockFixture, TestLockIXLockXRUnlock)
	{
		lock_protocol::Mode unused;
		global_hlckmgr->Acquire(root, lock_protocol::Mode::IX, 0);
		global_hlckmgr->Acquire(a, root, lock_protocol::Mode::XR, 0);
		CHECK(check_grant_x(region_, a) == 0);
		global_hlckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
	}
	
	TEST_FIXTURE(HLockFixture, TestLockIXLockXLUnlock)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		CHECK(global_hlckmgr->Acquire(root, lock_protocol::Mode::IX, 0) == lock_protocol::OK);
		EVENT("E2");
		CHECK(global_hlckmgr->Acquire(a, root, lock_protocol::Mode::XL, 0) == lock_protocol::OK);
		CHECK(check_grant_x(region_, a) == 0);
		global_hlckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
	}

	TEST_FIXTURE(HLockFixture, TestLockISLockSLUnlock)
	{
		lock_protocol::Mode unused;
		EVENT("E1");
		global_hlckmgr->Acquire(root, lock_protocol::Mode::IS, 0);
		global_hlckmgr->Acquire(a, root, lock_protocol::Mode::SL, 0);
		EVENT("E2");
		global_hlckmgr->Release(a);
		EVENT("E3");
	}
}
