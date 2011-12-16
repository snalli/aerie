#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "dpo/base/common/lock_protocol.h"
#include "dpo/base/client/hlckmgr.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "hlock.fixture.h"
#include "checklock.h"

using namespace client;

static dpo::cc::client::LockId root(1, 1);
static dpo::cc::client::LockId a(1, 2);
static dpo::cc::client::LockId b(1, 3);
static dpo::cc::client::LockId c(1, 4);


SUITE(HLock)
{
	TEST_FIXTURE(HLockFixture, TestLockIXLockXRUnlock)
	{
		lock_protocol::Mode unused;
		global_hlckmgr->Acquire(root, lock_protocol::Mode::IX, 0);
		global_hlckmgr->Acquire(a, root, lock_protocol::Mode::XR, 0);
		CHECK(check_grant_x(region_, a.number()) == 0);
		global_hlckmgr->Release(a);
		CHECK(check_release(region_, a.number()) == 0);
	}
	
	TEST_FIXTURE(HLockFixture, TestLockIXLockXLUnlock)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		printf("START\n");
		CHECK(global_hlckmgr->Acquire(root, lock_protocol::Mode::IX, 0) == lock_protocol::OK);
		printf("DONE\n");
		EVENT("E2");
		CHECK(global_hlckmgr->Acquire(a, root, lock_protocol::Mode::XL, 0) == lock_protocol::OK);
		CHECK(check_grant_x(region_, a.number()) == 0);
		global_hlckmgr->Release(a);
		CHECK(check_release(region_, a.number()) == 0);
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

	TEST_FIXTURE(HLockFixture, TestLockIXLockIXLockXLUnlockAll)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		CHECK(global_hlckmgr->Acquire(root, lock_protocol::Mode::IX, 0) == lock_protocol::OK);
		EVENT("E2");
		CHECK(global_hlckmgr->Acquire(a, root, lock_protocol::Mode::IX, 0) == lock_protocol::OK);
		EVENT("E3");
		CHECK(global_hlckmgr->Acquire(b, a, lock_protocol::Mode::XL, 0) == lock_protocol::OK);
		EVENT("E4");
		CHECK(check_grant_x(region_, b.number()) == 0);
		global_hlckmgr->Release(b);
		CHECK(check_release(region_, b.number()) == 0);
		global_hlckmgr->Release(a);
		global_hlckmgr->Release(root);
		EVENT("E5");
		EVENT("E6");
	}

	TEST_FIXTURE(HLockFixture, TestLockXR)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		CHECK(global_hlckmgr->Acquire(root, lock_protocol::Mode::XR, 0) == lock_protocol::OK);
		EVENT("E2");
	}
}
