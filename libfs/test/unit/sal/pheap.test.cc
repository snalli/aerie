#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "sal/pheap/pheap.h"

#include <stdio.h>

SUITE(SAL)
{
	TEST(TestPersistentHeap)
	{
		PersistentHeap* pheap;
		//CHECK(PersistentHeap::Create("/tmp/persistent_region1", 1024*1024) == E_SUCCESS);
		CHECK(PersistentHeap::Open("/tmp/persistent_heap1", &peap) == E_SUCCESS);
	}

}
