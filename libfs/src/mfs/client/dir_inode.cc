#include "mfs/client/dir_inode.h"
#include <stdint.h>
#include "client/inode.h"

namespace mfs {

//TODO: DirInodeImmutable: enables faster access for read-only operations.
// does an optimistic lookup and puts the result in inode.
// If a mutable version of the mutable exists then this function
// should fail and ask caller to do the normal slow path lookup

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

#if 0
// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}


int
namecmp(const char *s, const char *t)
{
  return strncmp(s, t, DIRSIZ);
}

// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
// Caller must have already locked dp.
struct inode*
dirlookup(struct inode *dp, char *name, uint *poff)
{
  uint off, inum;
  struct buf *bp;
  struct dirent *de;

  if(dp->type != T_DIR)
    panic("dirlookup not DIR");

  for(off = 0; off < dp->size; off += BSIZE){
    bp = bread(dp->dev, bmap(dp, off / BSIZE));
    for(de = (struct dirent*)bp->data;
        de < (struct dirent*)(bp->data + BSIZE);
        de++){
      if(de->inum == 0)
        continue;
      if(namecmp(name, de->name) == 0){
        // entry matches path element
        if(poff)
          *poff = off + (uchar*)de - bp->data;
        inum = de->inum;
        brelse(bp);
        return iget(dp->dev, inum);
      }
    }
    brelse(bp);
  }
  return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
int
dirlink(struct inode *dp, char *name, uint inum)
{
  int off;
  struct dirent de;
  struct inode *ip;
  int ret;

  // Check that name is not present.
  if((ip = dirlookup(dp, name, 0)) != 0){
    iput(ip);
    return -1;
  }

  // Look for an empty dirent.
  for(off = 0; off < dp->size; off += sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("dirlink read");
    if(de.inum == 0)
      break;
  }

  strncpy(de.name, name, DIRSIZ);
  de.inum = inum;
  if((ret = writei(dp, (char*)&de, off, sizeof(de))) != sizeof(de)) {
    panic("dirlink");
  }	
  
  return 0;
}

#endif


int 
DirInodeMutable::Lookup(client::Session* session, const char* name, client::Inode** ipp)
{
	int                   ret;
	uint64_t              ino;
	client::Inode*        ip;


	printf("DirInodeMutable::Lookup (%s) pnode_=%p\n", name, pnode_);

	if ((ret = pnode_->Lookup(session, name, &ino)) < 0) {
		return ret;
	}

	
	printf("DirInodeMutable::Lookup (%s): pnode_=%p, ino=%p\n", name, pnode_, ino);

	sb_->GetInode(ino, &ip);
	*ipp = ip;

	return 0;
}


int 
DirInodeMutable::Link(client::Session* session, const char* name, client::Inode* ip, 
                      bool overwrite)
{
	uint64_t ino;

	printf("DirInodeMutable::Link (%s)\n", name);

	if (name[0] == '\0') {
		return -1;
	}

	ino = ip->GetInodeNumber();

	printf("DirInodeMutable::Link (%s): pnode_=%p, ino=%p\n", name, pnode_, ino);
	return pnode_->Link(session, name, ino);
}


int 
DirInodeMutable::Publish()
{
	dbg_log (DBG_CRITICAL, "Functionality not yet implemented!\n");

	// TODO publish inode to the world
	// Need to merge the mutable region with the immutable pinode
	// Need to communicate with the file system server
}


} // namespace mfs
