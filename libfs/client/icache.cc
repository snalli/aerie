#include "client/icache.h"
#include <assert.h>
#include <map>


//TODO: optimization: use a per filesystem/inode type index
//TODO: optimization: for inodes that are locally mutable, use a 
// volatile field in the persistent inode to locate the volatile inode instead
// of a map index

int 
InodeCache::Lookup(uint64_t ino, client::Inode** inode)
{
	std::map<uint64_t, client::Inode*>::iterator it;

	it = ino2inode_map_.find(ino);

	if (it == ino2inode_map_.end()) {
		return -1;
	}
	*inode = it->second;
	return 0;
}


int 
InodeCache::Insert(client::Inode* inode)
{
	uint64_t                                              ino;
	std::pair<std::map<uint64_t, client::Inode*>::iterator, bool> pairret;

	printf("InodeCache::Insert (inode=%p)\n", inode);
	ino = inode->GetInodeNumber();
	printf("InodeCache::Insert <ino=0x%llx, inode=%p>\n", ino, inode);
	pairret = ino2inode_map_.insert(std::pair<uint64_t, client::Inode*>(ino, inode));
	assert(pairret.second == true);

	return 0;
}

int 
InodeCache::Remove(uint64_t ino)
{
	int ret;

	ret = ino2inode_map_.erase(ino);
	return ret;
}
