#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "ssa/main/client/rwproxy.h"
#include "ssa/main/client/omgr.h"
#include "ssa/main/client/registry.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/libfs.h"
#include "ssa.fixture.h"


SUITE(SSA_Registry)
{
	TEST_FIXTURE(SsaFixture, Test)
	{
		ssa::common::ObjectId tmp_oid;

		CHECK(global_storage_system->registry()->Add("/foo/bar", ssa::common::ObjectId(10)) == E_SUCCESS);
		CHECK(global_storage_system->registry()->Lookup("/foo/bar", &tmp_oid) == E_SUCCESS);
		CHECK(tmp_oid == ssa::common::ObjectId(10));
		CHECK(global_storage_system->registry()->Remove("/foo/bar") == E_SUCCESS);
		CHECK(global_storage_system->registry()->Lookup("/foo/bar", &tmp_oid) != E_SUCCESS);
	}

}
