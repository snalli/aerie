#include <stdlib.h>
#include "test/unittest.h"
#include "mfs/pinode.h"

#include <stdio.h>

extern void radix_tree_init_maxindex(void);

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
		pinode->WriteBlock(512*512+90, src, BLOCK_SIZE);
       	CHECK(pinode->get_size() == ((512*512+90+1)*BLOCK_SIZE));
		pinode->ReadBlock(12, dst, BLOCK_SIZE);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);
		pinode->ReadBlock(512, dst, BLOCK_SIZE);
       	CHECK(src[0] == 'a');
       	CHECK(src[0] == dst[0]);
		pinode->ReadBlock(512*512+81, dst, BLOCK_SIZE);
       	CHECK(src[0] == 'a');
       	CHECK(dst[0] == 0);

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

		for (int i=0; i<BLOCK_SIZE; i++) {
			src[i] = rand() % 256;
			zeros[i] = 0;
		}	
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
}
