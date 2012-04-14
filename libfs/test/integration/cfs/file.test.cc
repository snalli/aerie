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
static const char* test_str1 = "TEST_CREATE";

SUITE(CFSFile)
{
	TEST_FIXTURE(CFSFixture, TestCreate)
	{
		int  fd;
		char buf[512];

		EVENT("E1");
		CHECK(cfs_mount(storage_pool_path, "/", "cfs", 0) == 0);
		CHECK(cfs_mkdir("/dir", 0) == 0);
		CHECK((fd = cfs_open("/dir/file", O_CREAT|O_RDWR)) > 0);
		CHECK(cfs_write(fd, test_str1, strlen(test_str1)+1) > 0);
		CHECK(cfs_close(fd) == 0);
		EVENT("E2");
		EVENT("E3");
	}

	TEST_FIXTURE(CFSFixture, TestOpen)
	{
		int  fd;
		char buf[512];

		EVENT("E1");
		CHECK(cfs_mount(storage_pool_path, "/", "cfs", 0) == 0);
		CHECK((fd = cfs_open("/dir/file", O_RDWR)) > 0);
		CHECK(cfs_read(fd, buf, strlen(test_str1)+1) > 0);
		CHECK(strcmp(buf, test_str1) == 0);
		CHECK(cfs_close(fd) == 0);
		EVENT("E2");
		EVENT("E3");
	}
}
