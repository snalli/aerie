#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "ssa/main/common/lock_protocol.h"
#include "ssa/main/client/hlckmgr.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/libfs.h"
#include "lock.fixture.h"
#include "checklock.h"

using namespace client;

static ssa::cc::client::LockId root(1, 1);
static ssa::cc::client::LockId a(1, 2);
static ssa::cc::client::LockId b(1, 3);
static ssa::cc::client::LockId c(1, 4);


SUITE(SSA_HLock)
{
	TEST_FIXTURE(LockFixture, TestLockIXLockXRUnlock)
	{
		lock_protocol::Mode unused;
		global_storage_system->hlckmgr()->Acquire(root, lock_protocol::Mode::IX, 0);
		global_storage_system->hlckmgr()->Acquire(a, root, lock_protocol::Mode::XR, 0);
		CHECK(check_grant_x(region_, a.number()) == 0);
		global_storage_system->hlckmgr()->Release(a);
		CHECK(check_release(region_, a.number()) == 0);
	}
	
	TEST_FIXTURE(LockFixture, TestLockIXLockXLUnlock)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		printf("START\n");
		CHECK(global_storage_system->hlckmgr()->Acquire(root, lock_protocol::Mode::IX, 0) == lock_protocol::OK);
		printf("DONE\n");
		EVENT("E2");
		CHECK(global_storage_system->hlckmgr()->Acquire(a, root, lock_protocol::Mode::XL, 0) == lock_protocol::OK);
		CHECK(check_grant_x(region_, a.number()) == 0);
		global_storage_system->hlckmgr()->Release(a);
		CHECK(check_release(region_, a.number()) == 0);
	}

	TEST_FIXTURE(LockFixture, TestLockISLockSLUnlock)
	{
		lock_protocol::Mode unused;
		EVENT("E1");
		global_storage_system->hlckmgr()->Acquire(root, lock_protocol::Mode::IS, 0);
		global_storage_system->hlckmgr()->Acquire(a, root, lock_protocol::Mode::SL, 0);
		EVENT("E2");
		global_storage_system->hlckmgr()->Release(a);
		EVENT("E3");
	}

	TEST_FIXTURE(LockFixture, TestLockIXLockIXLockXLUnlockAll)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		CHECK(global_storage_system->hlckmgr()->Acquire(root, lock_protocol::Mode::IX, 0) == lock_protocol::OK);
		EVENT("E2");
		CHECK(global_storage_system->hlckmgr()->Acquire(a, root, lock_protocol::Mode::IX, 0) == lock_protocol::OK);
		EVENT("E3");
		CHECK(global_storage_system->hlckmgr()->Acquire(b, a, lock_protocol::Mode::XL, 0) == lock_protocol::OK);
		EVENT("E4");
		CHECK(check_grant_x(region_, b.number()) == 0);
		global_storage_system->hlckmgr()->Release(b);
		CHECK(check_release(region_, b.number()) == 0);
		global_storage_system->hlckmgr()->Release(a);
		global_storage_system->hlckmgr()->Release(root);
		EVENT("E5");
		EVENT("E6");
	}

	TEST_FIXTURE(LockFixture, TestLockXR)
	{
		lock_protocol::Mode unused;

		EVENT("E1");
		CHECK(global_storage_system->hlckmgr()->Acquire(root, lock_protocol::Mode::XR, 0) == lock_protocol::OK);
		EVENT("E2");
	}
}
