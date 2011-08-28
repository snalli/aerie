#include "mfs/imgr.h"
#include "client/backend.h"
#include "mfs/dinode.h"
#include "mfs/pstruct.h"

//TODO: Lock Inodes: short term latch (intra-process synchronization) 
//and lease lock (inter-process synchronization)

namespace mfs {

InodeManager::InodeManager(client::SuperBlock* sb, client::StorageManager* smgr)
	: timestamp_(0),
	  sb_(sb),
	  smgr_(smgr)
{
	imap_ = new client::InodeMap();
}


int
InodeManager::AllocInode(int type, client::Inode** ipp)
{
	client::Inode* ip;
	DirPnode*      dpnode;

	printf("mfs::InodeManager::AllocInode\n");
	
	switch (type) {
		case client::type::kFileInode:
			//ip = new 	
			break;
		case client::type::kDirInode:
			dpnode = new(smgr_) DirPnode;
			ip = new DirInodeMutable(sb_, dpnode);
			break;
	}

	*ipp = ip;

	return 0;
}


// If you ask for immutable inode then you should provide an Inode to wrap it in.
// If you ask for immutable inode but a mutable one exists in my cache then I won't provide it to you 
// If you ask for mutable inode then I will provide you with an Inode, which I will
// also be responsible for deallocating when its refcount drops to 0
int
InodeManager::GetInode(uint64_t ino, bool is_mutable, client::Inode** inodep)
{
	// check ino if in cache otherwise return immutable
	// if ino in cache then return mutable
	// how about race: someone gets immutable but someone gets mutable
	// and modifies. other doesn't see the modification.
	// you should provide an easy way to check whether immutable inode
	// is still valid. Here is a quick check that should work nicely in
	// absence of contention (design point case):

	int                ret;
	client::Inode*     inode;
	mfs::Pnode*        pnode;
	client::Inode*     iminode;
	client::Inode*     minode;

	if (!is_mutable) {
		if ((ret = imap_->Lookup(ino, &inode))==0) {
			return -1;
		}
		__sync_synchronize();
		if (*inodep) {
			return (*inodep)->Init(ino);
		} else {
			return -1;
		}
	}	
	
	if ((ret = imap_->Lookup(ino, &inode))==0) {
		//FIXME: bump the inode's ref count???
		*inodep = inode;
		return 0;
	}
	

	// Create a mutable inode and insert it into the inode cache
	// Bump the timestamp counter to signal to clients that an
	// immutable inode they hold may be invalid (because someone
	// else created a mutable one, which may match the one they hold)
	
	pnode = mfs::Pnode::Load(ino);
	//FIXME: persistent inode should have magic number identifying its type
	/*
	switch(pnode->magic_) {
		case 1: // directory
			minode = new mfs::DirInodeMutable(pnode);
			break;

		case 2: // file
			//FIXME
			//minode = new mfs::FileInodeMutable(pnode);
			break;
	}
	*/
	minode = new mfs::DirInodeMutable(sb_, pnode);
	printf("InodeManager::GetInode: sb_=%p\n", sb_);
	printf("InodeManager::GetInode: minode=%p\n", minode);
	printf("InodeManager::GetInode: minode->GetSuperBlock()=%p\n", minode->GetSuperBlock());

	//FIXME: what should be the inode's ref count??? 1?
	imap_->Insert(minode);
	__sync_fetch_and_add(&timestamp_, 1);
	*inodep = minode;

	return 0;
}

int
InodeManager::PutInode(client::Inode* inode)
{

	return 0;
}


}
