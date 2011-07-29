#include "mfs/imgr.h"
#include "mfs/inode.h"
#include "mfs/dinode.h"
#include "mfs/snode.h"

InodeManager::InodeManager()
	: timestamp_(0)
{

}


// If you ask for immutable inode then you should provide an Inode to wrap it in.
// If you ask for immutable inode but a mutable one exists in my cache then I won't provide it to you 
// If you ask for mutable inode then I will provide you with an Inode, which I will
// also be responsible for deallocating when its refcount drops to 0
int
InodeManager::GetInode(uint64_t ino, bool is_mutable, Inode** inodep)
{
	// check ino if in cache otherwise return immutable
	// if ino in cache then return mutable
	// how about race: someone gets immutable but someone gets mutable
	// and modifies. other doesn't see the modification.
	// you should provide an easy way to check whether immutable inode
	// is still valid. Here is a quick check that should work nicely in
	// absence of contention (design point case):

	int                ret;
	Inode*             inode;
	Snode*             snode;
	InodeImmutable*    iminode;
	InodeMutable*      minode;

	if (!is_mutable) {
		if ((ret = icache_->Lookup(ino, &inode))==0) {
			return -1;
		}
		__sync_synchronize();
		if (*inodep) {
			return (*inodep)->Init(ino);
		} else {
			return -1;
		}
	}	
	
	if ((ret = icache_->Lookup(ino, &inode))==0) {
		//FIXME: bump the inode's ref count???
		*inodep = inode;
		return 0;
	}
	

	// Create a mutable inode and insert it into the inode cache
	// Bump the timestamp counter to signal to clients that an
	// immutable inode they hold may be invalid (because someone
	// else created a mutable one, which may match the one they hold)
	
	snode = Snode::Load(ino);
	switch(snode->magic_) {
		case 1: // directory
			minode = new DirInodeMutable(snode);
			break;

		case 2: // file
			//TODO: file inode mutable 
			//minode = new FileInodeMutable(ino);
			break;
	}

	//FIXME: what should be the inode's ref count???
	icache_->Insert(minode);
	__sync_fetch_and_add(&timestamp_, 1);

	return 0;
}

int
InodeManager::PutInode(Inode* inode)
{


}
