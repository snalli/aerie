#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "cfs/client/client_i.h"
#include "cfs/client/libfs.h"
#include "cfs.fixture.h"

using namespace client;

static const char* storage_pool_path = "/tmp/stamnos_pool";

SUITE(CFS_Namespace)
{
	TEST_FIXTURE(CFSFixture, TestMkfs)
	{
		EVENT("MkfsAfter");
	}

	TEST_FIXTURE(CFSFixture, TestMount)
	{
		EVENT("MountBefore");
		CHECK(cfs_mount(storage_pool_path, "/", "cfs", 0) == 0);
		EVENT("MountAfter");
	}

	TEST_FIXTURE(CFSFixture, TestMkfsMkdir)
	{
		EVENT("E1");
		CHECK(cfs_mount(storage_pool_path, "/", "cfs", 0) == 0);
		CHECK(cfs_mkdir("/dir", 0) == 0);
		CHECK(cfs_mkdir("/dir/test", 0) == 0);
		EVENT("E2");
		EVENT("E3");
	}

	TEST_FIXTURE(CFSFixture, TestRmdir)
	{
		EVENT("E1");
		CHECK(cfs_mount(storage_pool_path, "/", "cfs", 0) == 0);
		EVENT("E2");
		CHECK(cfs_rmdir("/dir") != 0);
		CHECK(cfs_rmdir("/dir/test") == 0);
		CHECK(cfs_rmdir("/dir") == 0);
		EVENT("E3");
	}

	TEST_FIXTURE(CFSFixture, TestChdir)
	{
		EVENT("E1");
		CHECK(cfs_mount(storage_pool_path, "/home/hvolos", "cfs", 0) == 0);
		EVENT("E2");
		CHECK(cfs_chdir("/home/hvolos/dir") == 0);
		CHECK(cfs_chdir("test") == 0);
		EVENT("E3");
	}
}
