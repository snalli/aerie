#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "osd/main/client/rwproxy.h"
#include "osd/main/client/omgr.h"
#include "osd/main/client/registry.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/libfs.h"
#include "osd.fixture.h"


SUITE(OSD_Registry)
{
	TEST_FIXTURE(OsdFixture, Test)
	{
		osd::common::ObjectId tmp_oid;

		CHECK(global_storage_system->registry()->Add("/foo/bar", osd::common::ObjectId(10)) == E_SUCCESS);
		CHECK(global_storage_system->registry()->Lookup("/foo/bar", &tmp_oid) == E_SUCCESS);
		CHECK(tmp_oid == osd::common::ObjectId(10));
		CHECK(global_storage_system->registry()->Remove("/foo/bar") == E_SUCCESS);
		CHECK(global_storage_system->registry()->Lookup("/foo/bar", &tmp_oid) != E_SUCCESS);
	}

}
