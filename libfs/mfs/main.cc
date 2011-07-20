#include <stdio.h>
#include <iostream>
#include <assert.h>
#include "radixtree.h"
#include "common/hrtime.h"
#include "mfs/inode.h"
#include "mfs/hashtable.h"
#include "common/debug.h"

#define CHECK assert

#define min(a,b) ((a) < (b)? (a) : (b))
extern void radix_tree_init_maxindex();


void bar()
{
	int r;
	Bucket bk;
	uint64_t val;
	char *key1 = "key1";
	char *key2 = "key2";
	char *key3 = "key3";

	bk.Insert(key1, strlen(key1)+1, 0x9);
	bk.Insert(key2, strlen(key2)+1, 0x87);
	bk.Insert(key3, strlen(key3)+1, 0x33);

	r=bk.Search(key1, strlen(key1)+1, &val);
	printf("(r=%d), key=%s, val=%llx\n", r, key1, val);

	r=bk.Search(key2, strlen(key2)+1, &val);
	printf("(r=%d), key=%s, val=%llx\n", r, key2, val);

	r=bk.Search(key3, strlen(key3)+1, &val);
	printf("(r=%d), key=%s, val=%llx\n", r, key3, val);

	bk.Delete(key2, strlen(key2)+1);
	bk.Insert(key2, strlen(key2)+1, 0x56);

	r=bk.Search(key1, strlen(key1)+1, &val);
	printf("(r=%d), key=%s, val=%llx\n", r, key1, val);

	r=bk.Search(key2, strlen(key2)+1, &val);
	printf("(r=%d), key=%s, val=%llx\n", r, key2, val);

	r=bk.Search(key3, strlen(key3)+1, &val);
	printf("(r=%d), key=%s, val=%llx\n", r, key3, val);

}




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


}


main()
{
	radix_tree_init_maxindex();
	bar();
}
