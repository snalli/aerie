#ifndef _MFS_SUPERBLOCK_H_KAJ911
#define _MFS_SUPERBLOCK_H_KAJ911

#include "client/backend.h"
#include "mfs/dinode.h"
#include "mfs/pstruct.h"
#include "mfs/imgr.h"

namespace mfs {

class SuperBlock: public client::SuperBlock {
public:

	SuperBlock(PSuperBlock* psb)
		: psb_(psb),
		  root_(this, NULL)
	{ 
		imgr_ = new InodeManager(this, client::global_smgr);
		root_.Init(psb->root_);
		root_.SetSuperBlock(this);
		printf("Superblock: this=%p\n", this);
		printf("Superblock: root_=%p\n", &root_);
	}

	client::Inode* GetRootInode() {
		return &root_;
	}

	client::InodeManager* get_imgr() { return imgr_; }

	client::Inode* CreateImmutableInode(int t);
	client::Inode* CreateInode(int t);
	client::Inode* WrapInode();

	void* GetPSuperBlock() { return (void*) psb_; }

private:
	DirInodeMutable root_;
	PSuperBlock*    psb_;
};



} // namespace mfs

#endif /* _MFS_SUPERBLOCK_H_KAJ911 */
