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
#include "client/client_i.h"
#include "client/libfs.h"
#include "obj.fixture.h"


SUITE(Storage)
{
	TEST_FIXTURE(ObjectFixture, Test)
	{
		//FIXME
		//CHECK(libfs_mkfs("/superblock/A", "mfs", 1024, 0) == 0);
		//global_ssa_layer->salloc()->AllocateContainer(session);

	}

}
