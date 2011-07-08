#include <stdio.h>
#include <iostream>
#include <assert.h>
#include "radixtree.h"
#include "common/hrtime.h"
#include "mfs/inode.h"
#include "common/debug.h"

#define CHECK assert

#define min(a,b) ((a) < (b)? (a) : (b))
extern void radix_tree_init_maxindex();


void range(uint64_t off, uint64_t n)
{
	uint64_t tot;
	uint64_t m;
	uint64_t bn;
	uint64_t f;
	int      ret;
	int      i;

	for(tot=0, i=0; tot<n; tot+=m, off+=m, i++) {
		bn = off / BLOCK_SIZE;
		f = off % BLOCK_SIZE;
		m = min(n - tot, BLOCK_SIZE - f);
		printf("range: %d, R[%llu, %llu] A[%llu, %llu]\n", i, f, f+m-1, off, off+m-1);
	}
}



void foo()
{
	Inode*           inode;
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
	
	inode = new Inode(pinode);
	//inode->Write(src, 64, 4096*60);
	//inode->Write(src, 16*4096+64, 4096*60);
	inode->Write(src, (8+4*512*512LLU+512+2)*4096LLU, 4096*60);
	inode->Write(src, (8+4*512*512LLU+3*512+8)*4096LLU, 4096*60);
	inode->Read(src, (8+4*512*512LLU+3*512+8)*4096LLU, 4096*60);


}


main()
{
	radix_tree_init_maxindex();
	foo();
}
