#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "client/file.h"
#include "common/errno.h"

using namespace client;

SUITE(File)
{
	TEST(Alloc1)
	{
		File*        fp;
		FileManager* my_fmgr = new FileManager(1000, 2000);

		my_fmgr->Init();
		CHECK(my_fmgr->AllocFile(&fp)==0);
		CHECK(my_fmgr->AllocFd(fp)==1000);
		CHECK(my_fmgr->Put(1000)==0);
		CHECK(my_fmgr->Put(1000)==-1);
	}

	TEST(Alloc2)
	{
		File*        fp;
		FileManager* my_fmgr = new FileManager(1000, 2000);

		my_fmgr->Init();
		CHECK(my_fmgr->AllocFile(&fp)==0);
		CHECK(my_fmgr->AllocFd(fp)==1000);
		CHECK(my_fmgr->Get(1000, &fp)==1001);
		CHECK(my_fmgr->Put(1000)==0);
		CHECK(my_fmgr->Put(1001)==0);
	}
}
