#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include <iostream>
#include "chunkstore/chunkstore.h"
#include "client/nameservice.h"
#include "server/api.h"
#define stat xv6_stat  // avoid clash with host struct stat
#include "itree.h"
#include "types.h"
#include "fs.h"
#include "stat.h"


extern NameService* global_nameservice;
extern ChunkStore*  chunk_store;

#define BLOCK_SIZE BSIZE
#define INODE_BLOCK_SIZE 4096

int size = 4096;
int ninodes = 1024*16;

uint bitblocks;
uint freeinode = 1;

uint ialloc(ushort type);
void iappend(uint inum, void *p, int n);

#define min(a, b) ((a) < (b) ? (a) : (b))


int 
roundup(int size, int multiple)
{
	int rounded;

	rounded = (size % multiple == 0) ? size
	                                 : ((size / multiple) + 1) * multiple;
	return rounded;
}

#define INO(ADDR, INDEX)  ((((uint64_t) INDEX) << 48) | (ADDR))
#define INO2ADDR(INO)  (((1ULL << 48) - 1) & INO)
#define INO2INDEX(INO)  ((INO >> 48) & ((1 << 16) -1 ))

int
ialloc_bulk(void* itree)
{
	int              i;
	int              j;
	int              inode_size;
	ChunkDescriptor* chunkdsc;
	struct dinode*   din;
	ChunkDescriptor* chunkdsc_array[8];

	chunk_store->CreateChunk(INODE_BLOCK_SIZE, CHUNK_TYPE_INODE, &chunkdsc);
	chunk_store->AccessChunk(chunkdsc, PROT_WRITE|PROT_READ);

	inode_size = roundup(sizeof(dinode), 8);

	for (i=0, j=0; i<INODE_BLOCK_SIZE; i+=inode_size, j++) {
		din = (struct dinode*) (((uintptr_t) (chunkdsc->chunk_)) + i);
		//din->no = freeinode++;
		din->no = INO((uintptr_t) (chunkdsc), j);
		din->type =	T_FREE;   // mark inode as free? FIXME We probably need another way to
		                      // deduce whether an inode is free: e.g. no link to inode
	}

	assert(itree_insert_bulk(itree, chunkdsc) == 0);

	return 0;
}	



uint
ialloc(void* itree, ushort type)
{
	struct dinode*   din = NULL;
	int              ret;
	itree_entry*     ite;

	if (itree_find_inode(itree, 0, T_FREE, 0, &din) < 0) {
		ialloc_bulk(itree);
		ret = itree_find_inode(itree, 0, T_FREE, 0, &din);
	}
	assert(din);

	din->type = type;
	din->nlink = 1; //FIXME (DESIGN) do we need this??
	din->size = 0;
	return din->no;
}


void
iappend(void* itree, uint inum, void *xp, int n)
{
	char*            p = (char*)xp;
	uint             fbn;
	uint             off;
	uint             n1;
	struct dinode*   din;
	char             buf[BLOCK_SIZE];
	uint             indirect[NINDIRECT];
	uint64           x;
	int              ret;
	ChunkDescriptor* chunkdsc;
	ChunkDescriptor* chunkdsc_array[8];

	ret = itree_find_inode(itree, inum, 0, 0, &din);
	assert(ret == 0);
	assert(din->no == inum);

	off = din->size;

	while(n > 0) {
		fbn = off / BLOCK_SIZE;
		assert(fbn < MAXFILE);
		if(fbn < NDIRECT){
			if(din->addrs[fbn] == 0) {
				//FIXME: make allocation atomic
				chunk_store->CreateChunk(BLOCK_SIZE, CHUNK_TYPE_EXTENT, &chunkdsc);
				chunk_store->AccessChunk(chunkdsc, PROT_WRITE|PROT_READ);
				//FIXME: keep pointer to the descriptor instead
				din->addrs[fbn] = (uint64) chunkdsc->chunk_;
			}
			x = din->addrs[fbn];
		} else {
		  assert(0);
		  if(din->addrs[NDIRECT] == 0){
			//FIXME: make allocation atomic
			chunk_store->CreateChunk(BLOCK_SIZE, CHUNK_TYPE_EXTENT, &chunkdsc);
			din->addrs[NDIRECT] = (uint64) chunkdsc->chunk_;
		  }
		  //TODO: we don't need to copy in buffer and then back.
		  //We can do the modification in-place instead.
		  memcpy((char *) indirect, (void *) din->addrs[NDIRECT], BLOCK_SIZE);
		  if(indirect[fbn - NDIRECT] == 0){
			//FIXME: make allocation atomic
			chunk_store->CreateChunk(BLOCK_SIZE, CHUNK_TYPE_EXTENT, &chunkdsc);
			din->addrs[fbn - NDIRECT] = (uint64) chunkdsc->chunk_;
			memcpy((void *) din->addrs[NDIRECT], (char *) indirect, BLOCK_SIZE);
		  }
		  x = indirect[fbn-NDIRECT];
		}
		n1 = min(n, (fbn + 1) * BLOCK_SIZE - off);
		//memcpy(buf, (void *) x, BLOCK_SIZE);
		//bcopy(p, buf + off - (fbn * BLOCK_SIZE), n1);
		//memcpy((void *) x, buf, BLOCK_SIZE);
		memcpy((void *) (x + off - (fbn * BLOCK_SIZE)), (void*) p, n1);

		n -= n1;
		off += n1;
		p += n1;
	}
	din->size = off;
	//winode(inum, &din);
}


int
mkfs()
{
	int                i;
	int                cc;
	int                fd;
	uint               rootino;
	uint               inum;
	uint               off;
	struct dirent      de;
	char               buf[BLOCK_SIZE];
	struct dinode      din;
	int                usedblocks;
	ChunkDescriptor*   chunkdsc;
	ChunkDescriptor*   chunkdsc_sb;
	struct superblock* sb;

	//assert((BLOCK_SIZE % sizeof(struct dinode)) == 0);
	assert((BLOCK_SIZE % sizeof(struct dirent)) == 0);

	if (global_nameservice->Lookup("v6fs_superblock", (void**) &chunkdsc_sb) != 0) {
		// allocate and link must be atomic
		chunk_store->CreateChunk(4096, CHUNK_TYPE_SUPERBLOCK, &chunkdsc_sb);
		assert(global_nameservice->Link("v6fs_superblock", (void*) chunkdsc_sb) == 0);
		printf("chunkdsc_sb->chunk_ = %p\n", chunkdsc_sb->chunk_);
		printf("chunkdsc_sb->_size = %p\n", chunkdsc_sb->_size);
	}	
	sb = (struct superblock*) chunkdsc_sb->chunk_;
	chunk_store->AccessChunk(chunkdsc_sb, PROT_WRITE|PROT_READ);

	itree_create(&sb->itree);

	//chunk_store->CreateChunk(BLOCK_SIZE, CHUNK_TYPE_EXTENT, &chunkdsc);

	//sb->size = size;
	//sb->nblocks = nblocks;
	//sb->ninodes = ninodes;

	rootino = ialloc(sb->itree, T_DIR);

	bzero(&de, sizeof(de));
	de.inum = rootino;
	strcpy(de.name, ".");
	iappend(sb->itree, rootino, &de, sizeof(de));


	bzero(&de, sizeof(de));
	de.inum = rootino;
	strcpy(de.name, "..");
	iappend(sb->itree, rootino, &de, sizeof(de));

	exit(0);

	// fix size of root inode dir
	//rinode(rootino, &din);
	//off = din.size;
	//off = ((off/BSIZE) + 1) * BSIZE;
	//din.size = off;
	//winode(rootino, &din);

	exit(0);
}
