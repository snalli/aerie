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
//#include "checklock.hxx"

using namespace client;

static lock_protocol::LockId a = 1;
static lock_protocol::LockId b = 2;
static lock_protocol::LockId c = 3;

void test(char* tag)
{
	printf("%s\n", tag);

	global_lckmgr->Acquire(a, Lock::XL, 0);
	
	global_lckmgr->Release(a);
}
