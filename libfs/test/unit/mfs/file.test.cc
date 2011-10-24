#include <stdio.h>
#include <stdlib.h>
#include "tool/testfw/unittest.h"
#include "mfs/pinode.h"
#include "common/errno.h"

	
SUITE(FileInode)
{
	TEST(TestWriteReadBlock1)
	{
		/*
		FileInode*       inode;
		PInode*          pinode = new PInode;
		PInode::Iterator start(pinode, 0);
		PInode::Iterator iter;
		int              i;
		uint64_t         bn;
		char*            src;

		dbg_set_level(DBG_DEBUG);

		range(64, 60*4096);

		src = new char[600*4096];

		printf("base=%d, size=%d\n", (*start).base_bn_, 0);
		pinode->WriteBlock(src, 0, 0, 4096);
		//pinode->WriteBlock(src, 15, 0, 4096);
		pinode->WriteBlock(src, 8+4*512*512LLU+3*512+16, 0, 4096);
		//pinode->WriteBlock(src, 512+8, 0, 4096);

		inode = new FileInode(pinode);
		//inode->Write(src, 64, 4096*60);
		//inode->Write(src, 16*4096+64, 4096*60);
		inode->Write(src, (8+4*512*512LLU+512+2)*4096LLU, 4096*60);
		inode->Write(src, (8+4*512*512LLU+3*512+8)*4096LLU, 4096*60);
		inode->Read(src, (8+4*512*512LLU+3*512+8)*4096LLU, 4096*60);
		*/
	}
}
