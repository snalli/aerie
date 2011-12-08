#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
//#include "tool/testfw/integrationtest.h"
//#include "tool/testfw/testfw.h"
//#include "tool/testfw/ut_barrier.h"
#include "dpo/common/lock_protocol.h"
#include "dpo/client/hlckmgr.h"
#include "client/client_i.h"
#include "client/libfs.h"
//#include "checklock.hxx"

using namespace client;

static lock_protocol::LockId root = 1;
static lock_protocol::LockId a = 2;
static lock_protocol::LockId b = 3;
static lock_protocol::LockId c = 4;
static lock_protocol::LockId d = 5;
static lock_protocol::LockId e = 6;
static lock_protocol::LockId f = 7;
static lock_protocol::LockId g = 8;

void test1(const char* tag)
{
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IX, 0);
	global_hlckmgr->Acquire(a, root, lock_protocol::Mode::XR, 0);
	
	global_hlckmgr->Release(a);
}


void test2(const char* tag)
{
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IX, 0);
	global_hlckmgr->Acquire(a, root, lock_protocol::Mode::XR, 0);
	global_hlckmgr->Acquire(b, a, lock_protocol::Mode::XL, 0);
	
	global_hlckmgr->Release(b);
	global_hlckmgr->Release(a);

	global_hlckmgr->Acquire(a, root, lock_protocol::Mode::XL, 0);
	global_hlckmgr->Acquire(b, a, lock_protocol::Mode::XL, 0);
	
	global_hlckmgr->Release(b);
	global_hlckmgr->Release(a);

}

void test31(const char* tag)
{
	printf("test31\n");
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(a, root, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(b, a, lock_protocol::Mode::XL, 0);
	global_hlckmgr->Release(b);
	global_hlckmgr->Release(a);
	global_hlckmgr->Release(root);
	sleep(10);
}

void test32(const char* tag)
{
	printf("test32\n");
	sleep(1);
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(a, root, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(c, a, lock_protocol::Mode::XL, 0);
	//global_hlckmgr->Release(a);
	//global_hlckmgr->Release(root);
}


void test41(const char* tag)
{
	printf("%s\n", __FUNCTION__);
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(a, root, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(b, a, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(c, b, lock_protocol::Mode::XL, 0);
	//global_hlckmgr->Acquire(d, b, lock_protocol::Mode::XL, HLock::FLG_PUBLIC);
	global_hlckmgr->Acquire(d, b, lock_protocol::Mode::XL, 0);
	global_hlckmgr->Release(d);
	global_hlckmgr->Release(c);
	//global_hlckmgr->Release(b);
	//global_hlckmgr->Release(a);
	//global_hlckmgr->Release(root);

	printf("%s: DONE\n", __FUNCTION__);
	//global_hlckmgr->AcquireVector(, 0);
	//global_hlckmgr->Release(a);
	//global_hlckmgr->Release(root);
	sleep(1000);
}

void test42(const char* tag)
{
	printf("%s\n", __FUNCTION__);
	sleep(1);
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(e, root, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(d, e, lock_protocol::Mode::XL, HLock::FLG_PUBLIC);
	
	//global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(a, root, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(b, a, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Acquire(c, b, lock_protocol::Mode::XL, 0);

	printf("%s: DONE\n", __FUNCTION__);
}

void test51(const char* tag)
{
	printf("%s\n", __FUNCTION__);
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IXSL, 0);
	//global_hlckmgr->Acquire(a, root, lock_protocol::Mode::IXSL, 0);
	//global_hlckmgr->Acquire(b, a, lock_protocol::Mode::IXSL, 0);
	//global_hlckmgr->Acquire(c, b, lock_protocol::Mode::XL, 0);
	global_hlckmgr->Release(root);
	printf("%s: DONE\n", __FUNCTION__);
	sleep(1000);
}

void test52(const char* tag)
{
	printf("%s\n", __FUNCTION__);
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::XL, 0);
	printf("%s: DONE\n", __FUNCTION__);
}


void test(const char* tag) {
	if (strcmp(tag, "C1") == 0) {
		test31(tag);
	} else {
		test32(tag);
	}
}
