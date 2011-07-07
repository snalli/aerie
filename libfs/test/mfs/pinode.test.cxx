#include <stdlib.h>
#include "test/unittest.h"
#include "mfs/pinode.h"
#include "common/errno.h"

#include <stdio.h>

extern void radix_tree_init_maxindex(void);

void fillbuf(char *buf, int n, unsigned int seed)
{
	int i;

	srand(seed);
	for (int i=0; i<n; i++) {
		buf[i] = rand() % 256;
	}
}	


SUITE(SuitePInode)
{
	TEST(TestWriteReadBlock1)
	{
		PInode* pinode;
		char    src[BLOCK_SIZE];
		char    dst[BLOCK_SIZE];

		radix_tree_init_maxindex();

		pinode = new PInode();

		src[0] = 'a';
		pinode->WriteBlock(0, src, BLOCK_SIZE);
		pinode->ReadBlock(0, dst, BLOCK_SIZE);
       	CHECK(src[0] == dst[0]);

		delete pinode;
	}

	TEST(TestWriteReadBlock2)
	{
		PInode* pinode;
		char    src[BLOCK_SIZE];
		char    dst[BLOCK_SIZE];

		radix_tree_init_maxindex();

		pinode = new PInode();

		src[0] = 'a';
		pinode->WriteBlock(12, src, BLOCK_SIZE);
		pinode->WriteBlock(512, src, BLOCK_SIZE);
		pinode->WriteBlock(1024, src, BLOCK_SIZE);
		CHECK(pinode->WriteBlock(512*512+90, src, BLOCK_SIZE) > 0);
       	//CHECK(pinode->get_size() == ((512*512+90+1)*BLOCK_SIZE));

		CHECK(pinode->ReadBlock(12, dst, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);

		CHECK(pinode->ReadBlock(512, dst, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);

		CHECK(pinode->ReadBlock(1024, dst, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);


		CHECK(pinode->ReadBlock(512*512+90, dst, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);

		delete pinode;
	}

	TEST(TestWriteReadBlock3)
	{
		PInode* pinode;
		char    src[BLOCK_SIZE];
		char    dst[BLOCK_SIZE];
		char    zeros[BLOCK_SIZE];
		int     ret;

		radix_tree_init_maxindex();

		pinode = new PInode();

		memset(zeros, 0, BLOCK_SIZE);

		fillbuf(src, BLOCK_SIZE, 8);
		pinode->WriteBlock(8, src, 512);
       	CHECK(pinode->get_size() == (8*BLOCK_SIZE + 512));

		pinode->ReadBlock(5, dst, BLOCK_SIZE);
		CHECK(memcmp(dst, zeros, BLOCK_SIZE) == 0);

		ret=pinode->ReadBlock(8, dst, BLOCK_SIZE);
		CHECK(ret == 512);
		CHECK(memcmp(src, dst, 512) == 0);


		pinode->WriteBlock(98, src, 512);
       	CHECK(pinode->get_size() == (98*BLOCK_SIZE + 512));
		pinode->ReadBlock(8, dst, BLOCK_SIZE);
		CHECK(memcmp(dst, src, 512) == 0);
		CHECK(memcmp(&dst[512], zeros, BLOCK_SIZE-512) == 0);
		pinode->ReadBlock(65, dst, BLOCK_SIZE);
		CHECK(memcmp(dst, zeros, BLOCK_SIZE) == 0);
		ret=pinode->ReadBlock(98, dst, BLOCK_SIZE);
		CHECK(ret == 512);
		CHECK(memcmp(src, dst, 512) == 0);


		delete pinode;
	}


	TEST(TestWriteRead4)
	{
		PInode*          pinode;
		PInode::Iterator start;
		PInode::Iterator iter;
		char             src[BLOCK_SIZE];
		char             dst[BLOCK_SIZE];
		char             zeros[BLOCK_SIZE];
		int              ret;
		int              i;

		radix_tree_init_maxindex();

		pinode = new PInode();
		start.Init(pinode, 0);

		memset(zeros, 0, BLOCK_SIZE);

		fillbuf(src, BLOCK_SIZE, 8);
		pinode->WriteBlock(0, src, BLOCK_SIZE);
		pinode->WriteBlock(512+8, src, BLOCK_SIZE);
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


		delete pinode;
	}


	TEST(TestRegionWriteRead1)
	{
		PInode::Region* region;
		char            src[BLOCK_SIZE];
		char            dst[BLOCK_SIZE];
		char            zeros[BLOCK_SIZE];
		int             ret;

		radix_tree_init_maxindex();

		//region = new PInode::Region(0, RADIX_TREE_MAP_SIZE);
		region = new PInode::Region((uint64_t) 0, (uint64_t) 512);
		memset(zeros, 0, BLOCK_SIZE);

		fillbuf(src, BLOCK_SIZE, 0);
		CHECK(region->WriteBlock(0, src, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, RADIX_TREE_MAP_SIZE/2);
		CHECK(region->WriteBlock(RADIX_TREE_MAP_SIZE/2, src, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, RADIX_TREE_MAP_SIZE+1);
		CHECK(region->WriteBlock(RADIX_TREE_MAP_SIZE+1, src, BLOCK_SIZE) == -EINVAL);

		CHECK(region->ReadBlock(0, dst, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, 0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		CHECK(region->ReadBlock(RADIX_TREE_MAP_SIZE/2, dst, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, RADIX_TREE_MAP_SIZE/2);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		CHECK(region->ReadBlock(RADIX_TREE_MAP_SIZE+1, dst, BLOCK_SIZE) == -EINVAL);

		delete region;
	}


	TEST(TestInsertRegion1)
	{
		PInode*         pinode;
		PInode::Region* new_region;
		PInode::Region  region;
		char            src[BLOCK_SIZE];
		char            dst[BLOCK_SIZE];
		char            zeros[BLOCK_SIZE];
		int             ret;

		radix_tree_init_maxindex();

		pinode = new PInode();
		
		fillbuf(src, BLOCK_SIZE, 300);
		CHECK(pinode->WriteBlock(300, src, BLOCK_SIZE) >= 0);

		new_region = new PInode::Region(pinode, 400);
		fillbuf(src, BLOCK_SIZE, 400);
		CHECK(new_region->WriteBlock(400, src, BLOCK_SIZE) >= 0);
		
		CHECK(pinode->InsertRegion(new_region) == 0);
		CHECK(pinode->ReadBlock(400, dst, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		delete pinode;
	}
}
