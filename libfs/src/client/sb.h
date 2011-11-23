#ifndef _SUPERBLOCK_H_SHD191
#define _SUPERBLOCK_H_SHD191

#include "common/types.h"
#include "client/inode.h"

// The SuperBlock provides an indirection layer to the inodes of the 
// filesystem.
// We need indirection because a client cannot directly write to persistent 
// inodes. Instead we wrap a persistent inode into a local/private one that
// performs writes using copy-on-write (either logical or physical). 
// The SuperBlock is responsible for keeping track the index of inode number 
// to (volatile) inode. 


namespace client {

class Session; // forward declaration
class InodeMap; // forward declaration

class SuperBlock {
public:
	SuperBlock(Session* session);
	
	virtual void* GetPSuperBlock() = 0;
	int AllocInode(Session* session, int type, Inode** ipp);
	int GetInode(InodeNumber ino, Inode** ipp);
	int PutInode(Inode* ip);

	inline client::Inode* RootInode() {	return root_; }

protected:
	pthread_mutex_t mutex_;
	InodeMap*       imap_;
	Inode*          root_;

private:
	// FIXME: this functions should really be replaced with an inode factory 
	virtual int MakeInode(Session* session, int type, Inode** ipp) = 0;
	virtual int LoadInode(InodeNumber ino, client::Inode** ipp) = 0;
};



} // namespace client

#endif /* _SUPERBLOCK_H_SHD191 */
