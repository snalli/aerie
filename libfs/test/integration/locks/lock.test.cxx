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
	TEST_FIXTURE(LockFixture, TestLockUnlockConcurrentClients1)
	{
		CHECK(Client::TestServerIsAlive() == 0);
		ut_barrier_wait(&region_->barrier); 
		if (strcmp(TESTFW->Tag(), "C2")==0) {
			ut_barrier_wait(&region_->barrier); 
		}
		global_lckmgr->Acquire(a, Lock::XL, 0);
		CHECK(check_grant_x(region_, a) == 0);
		if (strcmp(TESTFW->Tag(), "C1")==0) {
			ut_barrier_wait(&region_->barrier); 
			usleep(100);
		}
		global_lckmgr->Release(a);
		CHECK(check_release(region_, a) == 0);
		ut_barrier_wait(&region_->barrier); 
	}

	TEST_FIXTURE(LockFixture, TestLockUnlockConcurrentClients2)
	{
		CHECK(Client::TestServerIsAlive() == 0);
		ut_barrier_wait(&region_->barrier); 
		for (int i=0; i<10; i++) {
			global_lckmgr->Acquire(a, Lock::XL, 0);
			CHECK(check_grant_x(region_, a) == 0);
			global_lckmgr->Release(a);
			CHECK(check_release(region_, a) == 0);
		}
		ut_barrier_wait(&region_->barrier); 
	}

	TEST_FIXTURE(LockFixture, TestLockUnlockConcurrentClients3)
	{
		CHECK(Client::TestServerIsAlive() == 0);
		for (int i=0; i<10; i++) {
			if (strcmp(TESTFW->Tag(), "C1")==0) {
				ut_barrier_wait(&region_->barrier); 
			}
			global_lckmgr->Acquire(a, Lock::XL, 0);
			if (strcmp(TESTFW->Tag(), "C2")==0) {
				ut_barrier_wait(&region_->barrier); 
			}
			CHECK(check_grant_x(region_, a) == 0);
			global_lckmgr->Release(a);
			CHECK(check_release(region_, a) == 0);
			ut_barrier_wait(&region_->barrier); 
		}
	}

/*
	TEST_THREAD_FIXTURE(LockFixture, TestLockUnlockConcurrentThreads, 2)
	{
		global_lckmgr->Acquire(a, Lock::IX);
		CHECK(check_grant_x(TEST_THREAD_SHARED->region_, a) == 0);
		global_lckmgr->Release(a);
		CHECK(check_release(TEST_THREAD_SHARED->region_, a) == 0);
	}
*/
}
