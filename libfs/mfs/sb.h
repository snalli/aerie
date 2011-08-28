#ifndef _MFS_SUPERBLOCK_H_KAJ911
#define _MFS_SUPERBLOCK_H_KAJ911

#include "client/backend.h"
#include "mfs/dinode.h"
#include "mfs/pstruct.h"

namespace mfs {

class SuperBlock: public client::SuperBlock {
public:

	SuperBlock(PSuperBlock* psb)
		: psb_(psb),
		  root_(this, NULL)
	{ 
		root_.Init(psb->root_);
		root_.SetSuperBlock(this);
		printf("Superblock: this=%p\n", this);
		printf("Superblock: root_=%p\n", &root_);
	}

	client::Inode* GetRootInode() {
		return &root_;
	}

	client::Inode* CreateImmutableInode(int t);
	client::Inode* WrapInode();
	int AllocInode(int type, client::Inode** ipp);
	int GetInode(client::InodeNumber ino, client::Inode** ipp);

	void* GetPSuperBlock() { return (void*) psb_; }

private:
	DirInodeMutable    root_;
	PSuperBlock*       psb_;
	client::InodeMap*  imap_;
};



} // namespace mfs

#endif /* _MFS_SUPERBLOCK_H_KAJ911 */
