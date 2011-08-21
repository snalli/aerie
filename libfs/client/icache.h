#ifndef _INODE_CACHE_H_AKL189
#define _INODE_CACHE_H_AKL189

#include "client/inode.h"
#include <stdint.h>
#include <map>

// This is an inode write-back cache that contains entries to be published. 

class InodeCache {

public:
	int Init();
	int Lookup(uint64_t ino, client::Inode** inode);
	int Insert(client::Inode* inode);
	int Remove(uint64_t ino);
	int RemoveAll();

private:
	
	std::map<uint64_t, client::Inode*> ino2inode_map_;
};

#endif
