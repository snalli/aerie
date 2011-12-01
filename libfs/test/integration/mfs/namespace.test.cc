#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rpc/rpc.h"
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "client/client_i.h"
#include "client/libfs.h"
#include "mfs.fixture.h"

using namespace client;

SUITE(Namespace)
{
	TEST_FIXTURE(MFSFixture, TestMount)
	{
		libfs_mkfs("/superblock/A", "mfs", 0);
		libfs_mount("/superblock/A", "/home/hvolos", "mfs", 0);
	}

	TEST(TestMkdir)
	{
		EVENT("E1");
		EVENT("E2");
		EVENT("E3");
	}

}
