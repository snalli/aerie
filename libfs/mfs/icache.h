#ifndef _INODE_CACHE_H_AKL189
#define _INODE_CACHE_H_AKL189

#include "mfs/inode.h"
#include <stdint.h>

// This is an inode write-back cache that contains entries to be published. 

class InodeCache {

public:
	int Init();
	int Lookup(uint64_t ino, Inode** inode);
	int Insert(Inode* inode);
	int Remove(uint64_t ino);
	int RemoveAll();

private:
	

	

};

#endif
