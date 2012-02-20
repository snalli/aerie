#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "test/unit/fixture/session.fixture.h"
#include "dpo/containers/byte/container.h"


using namespace dpo::containers::common;

const int BLOCK_SIZE = dpo::common::BLOCK_SIZE;

static void fillbuf(char *buf, int n, unsigned int seed)
{
	int i;

	srand(seed);
	for (int i=0; i<n; i++) {
		buf[i] = rand() % 256;
	}
}	


SUITE(ContainersByteContainer)
{
	TEST_FIXTURE(SessionFixture, TestWriteReadBlock1)
	{
		char                            src[BLOCK_SIZE];
		char                            dst[BLOCK_SIZE];
		ByteContainer::Object<Session>* byte_container = ByteContainer::Object<Session>::Make(session);

		src[0] = 'a';
		byte_container->WriteBlock(session, src, 0, 0, BLOCK_SIZE);
		byte_container->ReadBlock(session, dst, 0, 0, BLOCK_SIZE);
       	CHECK(src[0] == dst[0]);

		delete byte_container;
	}


	TEST_FIXTURE(SessionFixture, TestWriteReadBlock2)
	{
		char                            src[BLOCK_SIZE];
		char                            dst[BLOCK_SIZE];
		ByteContainer::Object<Session>* byte_container = ByteContainer::Object<Session>::Make(session);

		src[0] = 'a';
		byte_container->WriteBlock(session, src, 12, 0, BLOCK_SIZE);
		byte_container->WriteBlock(session, src, 512, 0, BLOCK_SIZE);
		byte_container->WriteBlock(session, src, 1024, 0, BLOCK_SIZE);
		CHECK(byte_container->WriteBlock(session, src, 512*512+90, 0, BLOCK_SIZE) > 0);

		CHECK(byte_container->ReadBlock(session, dst, 12, 0, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);

		CHECK(byte_container->ReadBlock(session, dst, 512, 0, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);

		CHECK(byte_container->ReadBlock(session, dst, 1024, 0, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);


		CHECK(byte_container->ReadBlock(session, dst, 512*512+90, 0, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);

		delete byte_container;
	}

	TEST_FIXTURE(SessionFixture, TestWriteReadBlock3)
	{
		char    src[BLOCK_SIZE];
		char    dst[BLOCK_SIZE];
		char    zeros[BLOCK_SIZE];
		int     ret;

		ByteContainer::Object<Session>* byte_container = ByteContainer::Object<Session>::Make(session);

		memset(zeros, 0, BLOCK_SIZE);

		fillbuf(src, BLOCK_SIZE, 8);
		byte_container->WriteBlock(session, src, 8, 0, 512);
       	CHECK(byte_container->Size() == (8*BLOCK_SIZE + 512));
		byte_container->ReadBlock(session, dst, 5, 0, BLOCK_SIZE);
		CHECK(memcmp(dst, zeros, BLOCK_SIZE) == 0);

		ret = byte_container->ReadBlock(session, dst, 8, 0, BLOCK_SIZE);
		CHECK(ret == 512);
		CHECK(memcmp(src, dst, 512) == 0);

		byte_container->WriteBlock(session, src, 98, 0, 512);
       	CHECK(byte_container->Size() == (98*BLOCK_SIZE + 512));
		byte_container->ReadBlock(session, dst, 8, 0, BLOCK_SIZE);
		CHECK(memcmp(dst, src, 512) == 0);
		CHECK(memcmp(&dst[512], zeros, BLOCK_SIZE-512) == 0);
		byte_container->ReadBlock(session, dst, 65, 0, BLOCK_SIZE);
		CHECK(memcmp(dst, zeros, BLOCK_SIZE) == 0);
		ret = byte_container->ReadBlock(session, dst, 98, 0, BLOCK_SIZE);
		CHECK(ret == 512);
		CHECK(memcmp(src, dst, 512) == 0);

		delete byte_container;
	}

	TEST_FIXTURE(SessionFixture, TestWriteRead4)
	{
		ByteContainer::Iterator<Session> start;
		ByteContainer::Iterator<Session> iter;
		char                             src[BLOCK_SIZE];
		char                             dst[BLOCK_SIZE];
		char                             zeros[BLOCK_SIZE];
		int                              ret;
		int                              i;
		ByteContainer::Object<Session>*  byte_container = ByteContainer::Object<Session>::Make(session);

		start.Init(session, byte_container, 0);

		memset(zeros, 0, BLOCK_SIZE);

		fillbuf(src, BLOCK_SIZE, 8);
		byte_container->WriteBlock(session, src, 0, 0, BLOCK_SIZE);
		byte_container->WriteBlock(session, src, 512+8, 0, BLOCK_SIZE);
		//for (iter = start, i=0; i< 11; iter++, i++) {
		for (iter = start, i=0; !iter.terminate(); iter++, i++) {
			if (i < 8) {
				CHECK((*iter).get_base_bn() == i);
			} else if (i==8) {
				CHECK((*iter).get_base_bn() == 8);
			} else if (i>=9 && i<= 521) {
				CHECK((*iter).get_base_bn() == i+511);
			} else if (i>521) {
				CHECK((*iter).get_base_bn() == 1032+512*(i-521));
			}
		}

		delete byte_container;
	}


	TEST_FIXTURE(SessionFixture, TestRegionWriteRead1)
	{
		char                            src[BLOCK_SIZE];
		char                            dst[BLOCK_SIZE];
		char                            zeros[BLOCK_SIZE];
		int                             ret;
		ByteContainer::Region<Session>* region = new ByteContainer::Region<Session>(session, (uint64_t) 0, (uint64_t) 512);

		memset(zeros, 0, BLOCK_SIZE);

		fillbuf(src, BLOCK_SIZE, 0);
		CHECK(region->WriteBlock(session, src, 0, 0, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, RADIX_TREE_MAP_SIZE/2);
		CHECK(region->WriteBlock(session, src, RADIX_TREE_MAP_SIZE/2, 0, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, RADIX_TREE_MAP_SIZE+1);
		CHECK(region->WriteBlock(session, src, RADIX_TREE_MAP_SIZE+1, 0, BLOCK_SIZE) >= 0);

		CHECK(region->ReadBlock(session, dst, 0, 0, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, 0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		CHECK(region->ReadBlock(session, dst, RADIX_TREE_MAP_SIZE/2, 0, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, RADIX_TREE_MAP_SIZE/2);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		CHECK(region->ReadBlock(session, dst, RADIX_TREE_MAP_SIZE+1, 0, BLOCK_SIZE) >= 0);

		delete region;
	}


	TEST_FIXTURE(SessionFixture, TestInsertRegion1)
	{
		char                            src[BLOCK_SIZE];
		char                            dst[BLOCK_SIZE];
		char                            zeros[BLOCK_SIZE];
		int                             ret;
		ByteContainer::Region<Session>* region;
		ByteContainer::Region<Session>* new_region;
		ByteContainer::Object<Session>* byte_container = ByteContainer::Object<Session>::Make(session);
		
		fillbuf(src, BLOCK_SIZE, 300);
		CHECK(byte_container->WriteBlock(session, src, 300, 0, BLOCK_SIZE) >= 0);

		new_region = new ByteContainer::Region<Session>(session, byte_container, (uint64_t) 400);
		fillbuf(src, BLOCK_SIZE, 400);
		CHECK(new_region->WriteBlock(session, src, 400, 0, BLOCK_SIZE) >= 0);
		
		CHECK(byte_container->InsertRegion(session, new_region) == 0);
		CHECK(byte_container->ReadBlock(session, dst, 400, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		delete byte_container;
	}


	TEST_FIXTURE(SessionFixture, TestExtendRegion1)
	{
		char                            src[BLOCK_SIZE];
		char                            dst[BLOCK_SIZE];
		char                            zeros[BLOCK_SIZE];
		int                             ret;
		ByteContainer::Region<Session>* region;
		ByteContainer::Region<Session>* new_region;
		ByteContainer::Object<Session>* byte_container = ByteContainer::Object<Session>::Make(session);
	
		new_region = new ByteContainer::Region<Session>(session, byte_container, (uint64_t) 400);

		fillbuf(src, BLOCK_SIZE, 400);
		CHECK(new_region->WriteBlock(session, src, 400, 0, BLOCK_SIZE) >= 0);
		
		CHECK(new_region->ReadBlock(session, dst, 400, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		// this implicitly extends the region
		fillbuf(src, BLOCK_SIZE, 670);
		CHECK(new_region->WriteBlock(session, src, 670, 0, BLOCK_SIZE) >= 0);

		fillbuf(src, BLOCK_SIZE, 400);
		CHECK(new_region->ReadBlock(session, dst, 400, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		fillbuf(src, BLOCK_SIZE, 670);
		CHECK(new_region->ReadBlock(session, dst, 670, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		// now insert the extended region to the byte_container
		CHECK(byte_container->InsertRegion(session, new_region) == 0);
		fillbuf(src, BLOCK_SIZE, 400);
		CHECK(byte_container->ReadBlock(session, dst, 400, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		fillbuf(src, BLOCK_SIZE, 670);
		CHECK(byte_container->ReadBlock(session, dst, 670, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		delete byte_container;
	}
}
