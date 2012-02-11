#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "dpo/main/client/rwproxy.h"
#include "dpo/main/client/omgr.h"
#include "dpo/main/client/smgr.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "obj.fixture.h"


SUITE(Storage)
{
	TEST_FIXTURE(ObjectFixture, Test)
	{
		global_dpo_layer->smgr()->AllocateContainerVector(session);

	}

}
