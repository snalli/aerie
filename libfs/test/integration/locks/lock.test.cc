#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "common/lock_protocol.h"
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
		global_lckmgr->Acquire(a, lock_protocol::Mode::SL, 0, unused);
		EVENT("E2");
	}
	
	TEST_FIXTURE(LockFixture, TestLockXLConvertSL)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);

		EVENT("E1");
		global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		global_lckmgr->Convert(a, lock_protocol::Mode::SL);
		EVENT("E3");
	}

	TEST_FIXTURE(LockFixture, TestLockXLConvertSLsynchronous)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);

		EVENT("E1");
		global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		global_lckmgr->Convert(a, lock_protocol::Mode::SL, true);
		EVENT("E3");
	}

	TEST_FIXTURE(LockFixture, TestLockUnlock)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		EVENT("E1");
		global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		CHECK(check_grant_x(region_, a) == 0);
		global_lckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
		EVENT("E3");
	}
	
	TEST_FIXTURE(LockFixture, TestLockUnlockTwoTimes)
	{
		lock_protocol::Mode unused;
		CHECK(Client::TestServerIsAlive() == 0);
		EVENT("E1");
		global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		EVENT("E2");
		CHECK(check_grant_x(region_, a) == 0);
		global_lckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
		global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
		CHECK(check_grant_x(region_, a) == 0);
		EVENT("E3");
		usleep(1000); // give enough time for a competing thread to try to acquire the lock
		              // we would like to be able to express this in our schedule
		global_lckmgr->Release(a);
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
			global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
			CHECK(check_grant_x(region_, a) == 0);
			EVENT("E3");
			global_lckmgr->Release(a);
			CHECK(check_release(region_, a) == 0);
			EVENT("E4");
		}
	}


	
/*
	// cancel request. deadlock scenario.
	TEST_THREAD_FIXTURE(LockFixture, TestLockCancel1, 2)
	{
		lock_protocol::Mode unused;

		if (strcmp(TESTFW->Tag(), "C1")==0) {
			// client 1. runs two threads
			if (TEST_THREAD_LOCAL->thread_id == 0) {
				// lock thread. acquires locks
				global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
				ut_barrier_wait(&TEST_THREAD_SHARED->region_->barrier); 
				ut_barrier_wait(TEST_THREAD_LOCAL->barrier); 
				global_lckmgr->Acquire(b, lock_protocol::Mode::XL, 0, unused);
				//sleep(1000);
			} else {
				// cancel thread. cancels outstanding lock requests
				ut_barrier_wait(TEST_THREAD_LOCAL->barrier); // wait till thread C1:0 stars the request
				usleep(100000); // let thread C1:0 make the request
				global_lckmgr->Cancel(b);
			}
		} else {
			// client 2. runs single thread
			if (TEST_THREAD_LOCAL->thread_id == 0) {
				global_lckmgr->Acquire(b, lock_protocol::Mode::XL, 0, unused);
				ut_barrier_wait(&TEST_THREAD_SHARED->region_->barrier); 
				global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
			}
		}
	}


	// cancel request
	TEST_THREAD_FIXTURE(LockFixture, TestLockCancel2, 2)
	{
		lock_protocol::Mode unused;
		if (strcmp(TESTFW->Tag(), "C1")==0) {
			// client 1 runs two threads
			if (TEST_THREAD_LOCAL->thread_id == 0) {
				// lock thread. acquires locks
				global_lckmgr->Acquire(a, lock_protocol::Mode::XL, 0, unused);
				ut_barrier_wait(&TEST_THREAD_SHARED->region_->barrier); // point 1: sync with thread C2:0
				ut_barrier_wait(TEST_THREAD_LOCAL->barrier); 
				CHECK(global_lckmgr->Acquire(b, lock_protocol::Mode::XL, 0, unused) == lock_protocol::DEADLK);
				usleep(500000);
			} else {
				// cancel thread. cancels outstanding lock requests
				ut_barrier_wait(TEST_THREAD_LOCAL->barrier); // wait till thread C1:0 starts the request
				usleep(1000);
				ut_barrier_wait(&TEST_THREAD_SHARED->region_->barrier); // point 2: sync with thread C2:0
				global_lckmgr->Cancel(b);
				ut_barrier_wait(&TEST_THREAD_SHARED->region_->barrier); // point 3: sync with thread C2:0
			}
		} else {
			// client 2 runs single thread
			if (TEST_THREAD_LOCAL->thread_id == 0) {
				global_lckmgr->Acquire(b, lock_protocol::Mode::XL, 0, unused);
				ut_barrier_wait(&TEST_THREAD_SHARED->region_->barrier); // point 1
				ut_barrier_wait(&TEST_THREAD_SHARED->region_->barrier); // point 2
				ut_barrier_wait(&TEST_THREAD_SHARED->region_->barrier); // point 3
				global_lckmgr->Release(b);
			}
		}
	}
*/

}
