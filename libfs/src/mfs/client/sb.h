#ifndef _MFS_SUPERBLOCK_H_KAJ911
#define _MFS_SUPERBLOCK_H_KAJ911

#include "client/backend.h"
#include "mfs/client/dinode.h"
#include "mfs/pstruct.h"

namespace mfs {

class SuperBlock: public client::SuperBlock {
public:

	SuperBlock(client::Session* session, PSuperBlock<client::Session>* psb)
		: psb_(psb),
		  root_(this, NULL)
	{ 
		root_.Init(session, psb->root_);
		root_.SetSuperBlock(this);
		imap_ = new client::InodeMap();
		printf("Superblock: this=%p\n", this);
		printf("Superblock: root_=%p\n", &root_);
	}

	client::Inode* GetRootInode() {
		return &root_;
	}

	client::Inode* CreateImmutableInode(int t);
	client::Inode* WrapInode();
	int AllocInode(client::Session* session, int type, client::Inode** ipp);
	int GetInode(client::InodeNumber ino, client::Inode** ipp);

	void* GetPSuperBlock() { return (void*) psb_; }

private:
	DirInodeMutable               root_;
	PSuperBlock<client::Session>* psb_;
	client::InodeMap*             imap_;
};



} // namespace mfs

#endif /* _MFS_SUPERBLOCK_H_KAJ911 */
