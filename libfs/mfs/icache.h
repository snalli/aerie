#ifndef _INODE_CACHE_H_AKL189
#define _INODE_CACHE_H_AKL189

// This is an inode write-back cache that contains entries to be published. 

class InodeCache {

public:
	int Init();
	int Lookup(uint64_t ino);
	int Insert(uint64_t ino);
	int Remove(uint64_t ino);
	int RemoveAll();

private:
	

	

};

#endif
