#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "dpo/main/client/rwproxy.h"
#include "dpo/main/client/omgr.h"
#include "dpo/main/client/registry.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "obj.fixture.h"


SUITE(Registry)
{
	TEST_FIXTURE(ObjectFixture, Test)
	{
		dpo::common::ObjectId tmp_oid;

		CHECK(global_dpo_layer->registry()->Add("/foo/bar", dpo::common::ObjectId(10)) == E_SUCCESS);
		CHECK(global_dpo_layer->registry()->Lookup("/foo/bar", &tmp_oid) == E_SUCCESS);
		CHECK(tmp_oid == dpo::common::ObjectId(10));
		CHECK(global_dpo_layer->registry()->Remove("/foo/bar") == E_SUCCESS);
		CHECK(global_dpo_layer->registry()->Lookup("/foo/bar", &tmp_oid) != E_SUCCESS);
	}

}
