#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "common/errno.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "osd/main/client/rwproxy.h"
#include "osd/main/client/omgr.h"
#include "osd/main/client/salloc.h"
#include "osd/main/client/shbuf.h"
#include "pxfs/client/client_i.h"
#include "pxfs/client/libfs.h"
#include "osd.fixture.h"


SUITE(OSD_StorageSystem)
{
	TEST_FIXTURE(OsdFixture, Test)
	{
		char buf[512];
		strcpy(buf, "str"); 
		global_storage_system->shbuf()->Write(buf, 512);
		global_storage_system->shbuf()->SignalReader();
	}

	TEST_FIXTURE(OsdFixture, AllocExtent)
	{
		osd::common::ExtentId eid;

		CHECK(global_storage_system->salloc()->AllocateExtent(session, 0, 4096, &eid) == E_SUCCESS);
		CHECK(global_storage_system->salloc()->AllocateExtent(session, 0, 4096, &eid) == E_SUCCESS);
	}

}
