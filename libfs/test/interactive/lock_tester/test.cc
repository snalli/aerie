#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
//#include "tool/testfw/integrationtest.h"
//#include "tool/testfw/testfw.h"
//#include "tool/testfw/ut_barrier.h"
#include "common/lock_protocol.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "client/hlckmgr.h"
//#include "checklock.hxx"

using namespace client;

static lock_protocol::LockId root = 1;
static lock_protocol::LockId a = 2;
static lock_protocol::LockId b = 3;
static lock_protocol::LockId c = 4;

void test1(char* tag)
{
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IX, 0);
	global_hlckmgr->Acquire(a, root, lock_protocol::Mode::XR, 0);
	
	global_hlckmgr->Release(a);
}


void test2(char* tag)
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

void test3(char* tag)
{
	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::IXSL, 0);
	global_hlckmgr->Release(root);

	global_hlckmgr->Acquire(root, 0, lock_protocol::Mode::XL, 0);
	global_hlckmgr->Release(root);
}

void test(char* tag) {
	test3(tag);
}
