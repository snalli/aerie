/// \file imap.h
/// 
/// \brief Inode Map: maps inode number to inode descriptor

#ifndef _INODE_MAP_H_AKL189
#define _INODE_MAP_H_AKL189

#include "client/inode.h"
#include <stdint.h>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/debug.h"

namespace client {

class InodeMap {

public:
	int Init();
	int Lookup(InodeNumber ino, Inode** inode);
	int Insert(Inode* inode);
	int Remove(InodeNumber ino);
	int RemoveAll();

private:
	
	google::dense_hash_map<InodeNumber, Inode*> ino2inode_map_;
};


inline int 
InodeMap::Lookup(InodeNumber ino, Inode** inode)
{
	google::dense_hash_map<InodeNumber, Inode*>::iterator it;

	it = ino2inode_map_.find(ino);

	if (it == ino2inode_map_.end()) {
		return -1;
	}
	*inode = it->second;
	return 0;
}


inline int 
InodeMap::Insert(Inode* inode)
{
	InodeNumber                                              ino;
	std::pair<google::dense_hash_map<InodeNumber, Inode*>::iterator, bool> pairret;

	ino = inode->GetInodeNumber();
	pairret = ino2inode_map_.insert(std::pair<InodeNumber, Inode*>(ino, inode));
	assert(pairret.second == true);

	return 0;
}

inline int 
InodeMap::Remove(InodeNumber ino)
{
	int ret;

	ret = ino2inode_map_.erase(ino);
	return ret;
}

} // namespace client

#endif
