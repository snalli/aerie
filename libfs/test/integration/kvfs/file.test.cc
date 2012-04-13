#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tool/testfw/integrationtest.h"
#include "tool/testfw/testfw.h"
#include "kvfs/client/client.h"
#include "kvfs/client/libfs.h"
#include "kvfs.fixture.h"

using namespace client;

static const char* storage_pool_path = "/tmp/stamnos_pool";
static const char* test_str1 = "TEST_CREATE";

SUITE(KVFSFile)
{
	TEST_FIXTURE(KVFSFixture, TestPut)
	{
		int  fd;
		char buf[512];

		EVENT("E1");
		CHECK(kvfs_mount(storage_pool_path, 0) == 0);
		CHECK(kvfs_put("test1", test_str1, strlen(test_str1)+1) > 0);
		CHECK(kvfs_get("test1", buf) > 0);
		CHECK(strcmp(buf, test_str1) == 0);
		CHECK(kvfs_sync() == 0);
		EVENT("E2");
		EVENT("E3");
	}

	TEST_FIXTURE(KVFSFixture, TestGet)
	{
		int  fd;
		char buf[512];

		EVENT("E1");
		CHECK(kvfs_mount(storage_pool_path, 0) == 0);
		//CHECK((fd = libfs_open("/home/hvolos/dir/file", O_RDWR)) > 0);
		//CHECK(libfs_read(fd, buf, strlen(test_str1)+1) > 0);
		//CHECK(strcmp(buf, test_str1) == 0);
		//CHECK(libfs_close(fd) == 0);
		EVENT("E2");
		EVENT("E3");
	}
}
