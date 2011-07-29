#ifndef _DIRECTORY_INODE_H_KAL178
#define _DIRECTORY_INODE_H_KAL178

#include "mfs/hashtable.h"
#include "mfs/inode.h"
#include "mfs/snode.h"
#include <stdint.h>

class DirInodeImmutable: public InodeImmutable {
public:

	DirInodeImmutable(Snode* snode)
		: snode_(static_cast<DirSnode*>(snode))
	{ }

	int Init(uint64_t ino) {
		snode_ = DirSnode::Load(ino);
		return 0;
	}
	
	//static DirInodeImmutable* Load(uint64_t ino);
	//static DirInodeImmutable* Load(InodeImmutable* inode);

	int Lookup(char* name);
	int Link(); // do nothing or don't expose this call
	int Unlink(); // do nothing or don't expose this call
	int Read(); 
	
private:
	DirSnode* snode_;

};

/*
DirInodeImmutable* 
DirInodeImmutable::Load(uint64_t ino)
{
	InodeImmutable* inode = InodeImmutable::ino2obj(ino);

	return DirInodeImmutable::Load(inode);
}


DirInodeImmutable* 
DirInodeImmutable::Load(InodeImmutable* inode)
{
	return reinterpret_cast<DirInodeImmutable*>(inode);
}
*/

//FIXME: directory needs a negative directory entry as well to
// indicate the absence of the entry
// if the entry is removed then the cache should reflect this
// (in contrast to the DLNC in Solaris and FreeBSD, the 
//  negative entry is necessary for correctness)


//TODO: keep a volatile hashtable per directory that represents the 
// up-to-date state of the directory???
// OR keep directory entries in a global cache (hashtable) where 
// each entry is indexed by (dirinode, ino)???
// What operations you need to perform on the volatile dir representation?
//  1) you need to quickly remove volatile entries of a directory inode (this is 
//     needed when publishing a directory inode as the persistent immutable 
//     inode becomes up-to-date and volatile entries are no-longer necessary)
//  2) add a directory entry
//  3) add a negative directory entry when removing a directory entry 
//  4) find a  directory entry given a directory inode and a name
//  5) Need to support listing all directory entries of a directory
//     This requires coordinating with the immutable directory inode. Simply
//     taking the union of the two inodes is not correct as some entries 
//     that appear in the persistent inode may have been removed and appear
//     as negative entries in the volatile cache. One way is for each entry 
//     in the persistent inode to check whether there is a corresponding negative
//     entry in the volatile cache. But this sounds like an overkill. 
//     Perhaps we need a combination of a bloom filter of just the negative entries 
//     and a counter of the negative entries. As deletes are rare, the bloom filter
//     should quickly provide an answer.

// TODO: perhaps it makes sense to cache persistent entries to avoid
// the second lookup (persistent inode lookup) for entries that have
// already been looked up in the past. But the benefit might not be much because
// it further slows down lookups as we need to insert an entry in the cache. 
 
// Lookup algorithm
// 1. Find whether there is a volatile directory inode or whether we should
//    query the persistent inode directly 


class DirInodeMutable: public InodeMutable {
public:
	DirInodeMutable(Snode* snode)
		: snode_(static_cast<DirSnode*>(snode))
	{ }


	int Init(uint64_t ino) {
		snode_ = DirSnode::Load(ino);
		return 0;
	}
	
private:
	DirSnode* snode_;
	//FIXME: pointer to new directory entries

};


#endif /* _DIRECTORY_INODE_H_KAL178 */
