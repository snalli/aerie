#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "tool/testfw/ut_barrier.h"
#include "common/lock_protocol.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "lock.fixture.hxx"
#include "checklock.hxx"

using namespace client;

static lock_protocol::LockId a = 1;
static lock_protocol::LockId b = 2;
static lock_protocol::LockId c = 3;


SUITE(Lock)
{
	TEST_FIXTURE(LockFixture, TestLockUnlockSingleClient1)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		CHECK(check_grant_x(region_, a) == 0);
		global_lckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
	}

	TEST_FIXTURE(LockFixture, TestLockUnlockSingleClient2)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		CHECK(check_grant_x(region_, a) == 0);
		global_lckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
	}

	TEST_FIXTURE(LockFixture, TestLockUnlockConcurrentClients1)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		ut_barrier_wait(&region_->barrier); 
		if (strcmp(TESTFW->Tag(), "C2")==0) {
			ut_barrier_wait(&region_->barrier); 
		}
		CHECK(global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused) == lock_protocol::OK);
		CHECK(check_grant_x(region_, a) == 0);
		if (strcmp(TESTFW->Tag(), "C1")==0) {
			ut_barrier_wait(&region_->barrier); 
			usleep(100);
		}
		CHECK(global_lckmgr->Release(a) == lock_protocol::OK);
		CHECK(check_release(region_, a) == 0);
		ut_barrier_wait(&region_->barrier); 
	}

	TEST_FIXTURE(LockFixture, TestLockUnlockConcurrentClients2)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		ut_barrier_wait(&region_->barrier); 
		for (int i=0; i<10; i++) {
			global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
			CHECK(check_grant_x(region_, a) == 0);
			global_lckmgr->Release(a);
			CHECK(check_release(region_, a) == 0);
		}
		ut_barrier_wait(&region_->barrier); 
	}

	TEST_FIXTURE(LockFixture, TestLockUnlockConcurrentClients3)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		for (int i=0; i<10; i++) {
			if (strcmp(TESTFW->Tag(), "C1")==0) {
				ut_barrier_wait(&region_->barrier); 
			}
			global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
			if (strcmp(TESTFW->Tag(), "C2")==0) {
				ut_barrier_wait(&region_->barrier); 
			}
			CHECK(check_grant_x(region_, a) == 0);
			global_lckmgr->Release(a);
			CHECK(check_release(region_, a) == 0);
			ut_barrier_wait(&region_->barrier); 
		}
	}

	// checks that a client that grabs a cached lock is serialized
	// with respect to another client trying to acquire the same lock
	TEST_FIXTURE(LockFixture, TestLockUnlockConcurrentClients4)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);

		if (strcmp(TESTFW->Tag(), "C1")==0) {
			ut_barrier_wait(&region_->barrier); 
			global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
			CHECK(check_grant_x(region_, a) == 0);
			global_lckmgr->Release(a);
			CHECK(check_release(region_, a) == 0);
		} else if (strcmp(TESTFW->Tag(), "C2")==0) {
			// the second acquire grabs the cached lock
			global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
			CHECK(check_grant_x(region_, a) == 0);
			global_lckmgr->Release(a);
			CHECK(check_release(region_, a) == 0);
			global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused); // second acquire
			CHECK(check_grant_x(region_, a) == 0);
			ut_barrier_wait(&region_->barrier); 
			usleep(1000); // give enough time for the competing thread to try to acquire the lock
			global_lckmgr->Release(a);
			CHECK(check_release(region_, a) == 0);
		}
		ut_barrier_wait(&region_->barrier); 
	}

/*
	TEST_THREAD_FIXTURE(LockFixture, TestLockUnlockConcurrentThreads, 2)
	{
		global_lckmgr->Acquire(a, lock_protocol::Mode::IX);
		CHECK(check_grant_x(TEST_THREAD_SHARED->region_, a) == 0);
		global_lckmgr->Release(a);
		CHECK(check_release(TEST_THREAD_SHARED->region_, a) == 0);
	}
*/
}
