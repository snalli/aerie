#include "client/imgr.h"
#include "client/inode.h"
#include "mfs/dinode.h"
#include "mfs/pstruct.h"

//TODO: Lock Inodes: short term latch (intra-process synchronization) 
//and lease lock (inter-process synchronization)

#if 0
client::InodeManager::InodeManager()
	: timestamp_(0)
{

}

int
client::InodeManager::AllocInode(Inode** inodep)
{

}


// If you ask for immutable inode then you should provide an Inode to wrap it in.
// If you ask for immutable inode but a mutable one exists in my cache then I won't provide it to you 
// If you ask for mutable inode then I will provide you with an Inode, which I will
// also be responsible for deallocating when its refcount drops to 0
int
client::InodeManager::GetInode(uint64_t ino, bool is_mutable, Inode** inodep)
{
}

int
client::InodeManager::PutInode(Inode* inode)
{


}


#endif
