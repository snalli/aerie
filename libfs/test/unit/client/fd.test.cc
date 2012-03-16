#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "pxfs/client/file.h"
#include "common/errno.h"

using namespace client;

SUITE(FileDescriptor)
{
	TEST(AllocSingle)
	{
		File*        fp;
		FileManager* my_fmgr = new FileManager(1000, 2000);

		my_fmgr->Init();
		CHECK(my_fmgr->AllocFd(fp)==1000);
	}

	TEST(AllocMultiple1)
	{
		File*        fp;
		FileManager* my_fmgr = new FileManager(1000, 1004);

		my_fmgr->Init();
		CHECK(my_fmgr->AllocFd(fp)==1000);
		CHECK(my_fmgr->AllocFd(fp)==1001);
		CHECK(my_fmgr->AllocFd(fp)==1002);
		CHECK(my_fmgr->AllocFd(fp)==1003);
		CHECK(my_fmgr->AllocFd(fp) < 0);
		CHECK(my_fmgr->Put(999) < 0);
		CHECK(my_fmgr->Put(1004) < 0);
	}

	TEST(AllocMultiple2)
	{
		File*        fp;
		FileManager* my_fmgr = new FileManager(1000, 1004);

		my_fmgr->Init();
		my_fmgr->AllocFile(&fp);
		CHECK(my_fmgr->AllocFd(fp)==1000);
		CHECK(my_fmgr->AllocFd(fp)==1001);
		CHECK(my_fmgr->Put(1000)==0);
		CHECK(my_fmgr->AllocFd(fp)==1000);
	}
}
