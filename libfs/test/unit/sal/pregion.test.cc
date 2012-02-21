#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "sal/pregion/pregion.h"

#include <stdio.h>

SUITE(SAL)
{
	TEST(TestPersistentRegion)
	{
		PersistentRegion* pregion1;
		PersistentRegion* pregion2;
		CHECK(PersistentRegion::Create("/tmp/persistent_region1", 1024*1024) == E_SUCCESS);
		CHECK(PersistentRegion::Create("/tmp/persistent_region2", 1024*1024) == E_SUCCESS);
		CHECK(PersistentRegion::Open("/tmp/persistent_region1", &pregion1) == E_SUCCESS);
		CHECK(pregion1->base() == 0x100000000000LLU);
		CHECK(PersistentRegion::Open("/tmp/persistent_region2", &pregion2) == E_SUCCESS);
		CHECK(pregion2->base() == 0x100000100000LLU);
		CHECK(PersistentRegion::Close(pregion1) == E_SUCCESS);
		CHECK(PersistentRegion::Close(pregion2) == E_SUCCESS);

		// Remap: region1, then region2
		CHECK(PersistentRegion::Open("/tmp/persistent_region1", &pregion1) == E_SUCCESS);
		CHECK(pregion1->base() == 0x100000000000LLU);
		CHECK(PersistentRegion::Open("/tmp/persistent_region2", &pregion2) == E_SUCCESS);
		CHECK(pregion2->base() == 0x100000100000LLU);
		CHECK(PersistentRegion::Close(pregion1) == E_SUCCESS);
		CHECK(PersistentRegion::Close(pregion2) == E_SUCCESS);
		
		// Remap: region2, then region1
		CHECK(PersistentRegion::Open("/tmp/persistent_region2", &pregion2) == E_SUCCESS);
		CHECK(pregion2->base() == 0x100000100000LLU);
		CHECK(PersistentRegion::Open("/tmp/persistent_region1", &pregion1) == E_SUCCESS);
		CHECK(pregion1->base() == 0x100000000000LLU);
		CHECK(PersistentRegion::Close(pregion1) == E_SUCCESS);
		CHECK(PersistentRegion::Close(pregion2) == E_SUCCESS);
	}

}
