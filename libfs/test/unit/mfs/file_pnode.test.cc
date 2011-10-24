#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "mfs/file_pnode.h"
#include "common/errno.h"


extern void radix_tree_init_maxindex(void);

void fillbuf(char *buf, int n, unsigned int seed)
{
	int i;

	srand(seed);
	for (int i=0; i<n; i++) {
		buf[i] = rand() % 256;
	}
}	


SUITE(SuiteFilePnode)
{
	TEST(TestWriteReadBlock1)
	{
		FilePnode* pinode;
		char    src[BLOCK_SIZE];
		char    dst[BLOCK_SIZE];

		radix_tree_init_maxindex();

		pinode = new FilePnode();

		src[0] = 'a';
		pinode->WriteBlock(src, 0, 0, BLOCK_SIZE);
		pinode->ReadBlock(dst, 0, 0, BLOCK_SIZE);
       	CHECK(src[0] == dst[0]);

		delete pinode;
	}

	TEST(TestWriteReadBlock2)
	{
		FilePnode* pinode;
		char    src[BLOCK_SIZE];
		char    dst[BLOCK_SIZE];

		radix_tree_init_maxindex();

		pinode = new FilePnode();

		src[0] = 'a';
		pinode->WriteBlock(src, 12, 0, BLOCK_SIZE);
		pinode->WriteBlock(src, 512, 0, BLOCK_SIZE);
		pinode->WriteBlock(src, 1024, 0, BLOCK_SIZE);
		CHECK(pinode->WriteBlock(src, 512*512+90, 0, BLOCK_SIZE) > 0);

		CHECK(pinode->ReadBlock(dst, 12, 0, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);

		CHECK(pinode->ReadBlock(dst, 512, 0, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);

		CHECK(pinode->ReadBlock(dst, 1024, 0, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);


		CHECK(pinode->ReadBlock(dst, 512*512+90, 0, BLOCK_SIZE) > 0);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);

		delete pinode;
	}

	TEST(TestWriteReadBlock3)
	{
		FilePnode* pinode;
		char    src[BLOCK_SIZE];
		char    dst[BLOCK_SIZE];
		char    zeros[BLOCK_SIZE];
		int     ret;

		radix_tree_init_maxindex();

		pinode = new FilePnode();

		memset(zeros, 0, BLOCK_SIZE);

		fillbuf(src, BLOCK_SIZE, 8);
		pinode->WriteBlock(src, 8, 0, 512);
       	CHECK(pinode->get_size() == (8*BLOCK_SIZE + 512));

		pinode->ReadBlock(dst, 5, 0, BLOCK_SIZE);
		CHECK(memcmp(dst, zeros, BLOCK_SIZE) == 0);

		ret=pinode->ReadBlock(dst, 8, 0, BLOCK_SIZE);
		CHECK(ret == 512);
		CHECK(memcmp(src, dst, 512) == 0);


		pinode->WriteBlock(src, 98, 0, 512);
       	CHECK(pinode->get_size() == (98*BLOCK_SIZE + 512));
		pinode->ReadBlock(dst, 8, 0, BLOCK_SIZE);
		CHECK(memcmp(dst, src, 512) == 0);
		CHECK(memcmp(&dst[512], zeros, BLOCK_SIZE-512) == 0);
		pinode->ReadBlock(dst, 65, 0, BLOCK_SIZE);
		CHECK(memcmp(dst, zeros, BLOCK_SIZE) == 0);
		ret=pinode->ReadBlock(dst, 98, 0, BLOCK_SIZE);
		CHECK(ret == 512);
		CHECK(memcmp(src, dst, 512) == 0);


		delete pinode;
	}


	TEST(TestWriteRead4)
	{
		FilePnode*          pinode;
		FilePnode::Iterator start;
		FilePnode::Iterator iter;
		char             src[BLOCK_SIZE];
		char             dst[BLOCK_SIZE];
		char             zeros[BLOCK_SIZE];
		int              ret;
		int              i;

		radix_tree_init_maxindex();

		pinode = new FilePnode();
		start.Init(pinode, 0);

		memset(zeros, 0, BLOCK_SIZE);

		fillbuf(src, BLOCK_SIZE, 8);
		pinode->WriteBlock(src, 0, 0, BLOCK_SIZE);
		pinode->WriteBlock(src, 512+8, 0, BLOCK_SIZE);
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
		FilePnode::Region* region;
		char            src[BLOCK_SIZE];
		char            dst[BLOCK_SIZE];
		char            zeros[BLOCK_SIZE];
		int             ret;

		radix_tree_init_maxindex();

		//region = new FilePnode::Region(0, RADIX_TREE_MAP_SIZE);
		region = new FilePnode::Region((uint64_t) 0, (uint64_t) 512);
		memset(zeros, 0, BLOCK_SIZE);

		fillbuf(src, BLOCK_SIZE, 0);
		CHECK(region->WriteBlock(src, 0, 0, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, RADIX_TREE_MAP_SIZE/2);
		CHECK(region->WriteBlock(src, RADIX_TREE_MAP_SIZE/2, 0, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, RADIX_TREE_MAP_SIZE+1);
		CHECK(region->WriteBlock(src, RADIX_TREE_MAP_SIZE+1, 0, BLOCK_SIZE) >= 0);

		CHECK(region->ReadBlock(dst, 0, 0, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, 0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		CHECK(region->ReadBlock(dst, RADIX_TREE_MAP_SIZE/2, 0, BLOCK_SIZE) >= 0);
		fillbuf(src, BLOCK_SIZE, RADIX_TREE_MAP_SIZE/2);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		CHECK(region->ReadBlock(dst, RADIX_TREE_MAP_SIZE+1, 0, BLOCK_SIZE) >= 0);

		delete region;
	}


	TEST(TestInsertRegion1)
	{
		FilePnode*         pinode;
		FilePnode::Region* new_region;
		FilePnode::Region  region;
		char            src[BLOCK_SIZE];
		char            dst[BLOCK_SIZE];
		char            zeros[BLOCK_SIZE];
		int             ret;

		radix_tree_init_maxindex();

		pinode = new FilePnode();
		
		fillbuf(src, BLOCK_SIZE, 300);
		CHECK(pinode->WriteBlock(src, 300, 0, BLOCK_SIZE) >= 0);

		new_region = new FilePnode::Region(pinode, 400);
		fillbuf(src, BLOCK_SIZE, 400);
		CHECK(new_region->WriteBlock(src, 400, 0, BLOCK_SIZE) >= 0);
		
		CHECK(pinode->InsertRegion(new_region) == 0);
		CHECK(pinode->ReadBlock(dst, 400, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		delete pinode;
	}

	TEST(TestExtendRegion1)
	{
		FilePnode*         pinode;
		FilePnode::Region* new_region;
		FilePnode::Region  region;
		char            src[BLOCK_SIZE];
		char            dst[BLOCK_SIZE];
		char            zeros[BLOCK_SIZE];
		int             ret;

		radix_tree_init_maxindex();

		pinode = new FilePnode();
		
		new_region = new FilePnode::Region(pinode, 400);

		fillbuf(src, BLOCK_SIZE, 400);
		CHECK(new_region->WriteBlock(src, 400, 0, BLOCK_SIZE) >= 0);
		
		CHECK(new_region->ReadBlock(dst, 400, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		// this implicitly extends the region
		fillbuf(src, BLOCK_SIZE, 670);
		CHECK(new_region->WriteBlock(src, 670, 0, BLOCK_SIZE) >= 0);

		fillbuf(src, BLOCK_SIZE, 400);
		CHECK(new_region->ReadBlock(dst, 400, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		fillbuf(src, BLOCK_SIZE, 670);
		CHECK(new_region->ReadBlock(dst, 670, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		// now insert the extended region to the pinode
		CHECK(pinode->InsertRegion(new_region) == 0);
		fillbuf(src, BLOCK_SIZE, 400);
		CHECK(pinode->ReadBlock(dst, 400, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		fillbuf(src, BLOCK_SIZE, 670);
		CHECK(pinode->ReadBlock(dst, 670, 0, BLOCK_SIZE) >=0);
		CHECK(memcmp(dst, src, BLOCK_SIZE) == 0);

		delete pinode;
	}

}
