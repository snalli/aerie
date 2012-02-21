#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "sal/pool/pool.h"

#include <stdio.h>

SUITE(SAL)
{
	TEST(TestPool)
	{
		StoragePool* pool1;
		void*        ptr;

		CHECK(StoragePool::Create("/tmp/persistent_pool1", 1024*4096) == E_SUCCESS);
		CHECK(StoragePool::Open("/tmp/persistent_pool1", &pool1) == E_SUCCESS);

		CHECK(pool1->AllocateExtent(4096*2, &ptr) == E_SUCCESS);
		CHECK(pool1->AllocateExtent(4096*5, &ptr) == E_SUCCESS);
		
		CHECK(StoragePool::Close(pool1) == E_SUCCESS);

		CHECK(StoragePool::Open("/tmp/persistent_pool1", &pool1) == E_SUCCESS);
		CHECK(pool1->AllocateExtent(4096*3, &ptr) == E_SUCCESS);
	}
}
