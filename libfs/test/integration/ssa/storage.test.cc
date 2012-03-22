#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "ssa/main/client/rwproxy.h"
#include "ssa/main/client/omgr.h"
#include "ssa/main/client/salloc.h"
#include "ssa/main/client/shbuf.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/libfs.h"
#include "ssa.fixture.h"


SUITE(SSA_StorageSystem)
{
	TEST_FIXTURE(SsaFixture, Test)
	{
		char buf[512];
		strcpy(buf, "str"); 
		global_storage_system->shbuf()->Write(buf, 512);
		global_storage_system->shbuf()->SignalReader();
	}

}
