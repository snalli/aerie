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
	global_hlckmgr->Acquire(root, 0, HLock::IX, 0);
	global_hlckmgr->Acquire(a, root, HLock::XR, 0);
	
	global_hlckmgr->Release(a);
}


void test2(char* tag)
{
	global_hlckmgr->Acquire(root, 0, HLock::IX, 0);
	global_hlckmgr->Acquire(a, root, HLock::XR, 0);
	global_hlckmgr->Acquire(b, a, HLock::XL, 0);
	
	global_hlckmgr->Release(b);
	global_hlckmgr->Release(a);

	global_hlckmgr->Acquire(a, root, HLock::XL, 0);
	global_hlckmgr->Acquire(b, a, HLock::XL, 0);
	
	global_hlckmgr->Release(b);
	global_hlckmgr->Release(a);

}


void test(char* tag) {
	test2(tag);
}
