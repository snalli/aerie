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
#include "client/client_i.h"
#include "client/libfs.h"
#include "obj.fixture.h"


SUITE(Registry)
{
	TEST_FIXTURE(ObjectFixture, Test)
	{
		ssa::common::ObjectId tmp_oid;

		CHECK(global_ssa_layer->registry()->Add("/foo/bar", ssa::common::ObjectId(10)) == E_SUCCESS);
		CHECK(global_ssa_layer->registry()->Lookup("/foo/bar", &tmp_oid) == E_SUCCESS);
		CHECK(tmp_oid == ssa::common::ObjectId(10));
		CHECK(global_ssa_layer->registry()->Remove("/foo/bar") == E_SUCCESS);
		CHECK(global_ssa_layer->registry()->Lookup("/foo/bar", &tmp_oid) != E_SUCCESS);
	}

}
