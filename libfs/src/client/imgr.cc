#include "client/imgr.h"
#include "client/sb.h"

// TODO: Lock Inodes via Lock Manager

// TODO: optimize immutable inodes (e.g. for lookup)
// check ino if in cache otherwise return immutable
// if ino in cache then return mutable
// how about race: someone gets immutable but someone gets mutable
// and modifies. other doesn't see the modification.
// you should provide an easy way to check whether immutable inode
// is still valid. Here is a quick check that should work nicely in
// absence of contention (design point case):
//
// For example, GetInode should:
// -If asked for immutable inode then user should provide an Inode to wrap it in.
// -If asked for immutable inode then a mutable one exists in my cache then I won't provide it to you 
// -If asked for mutable inode then I will provide you with an Inode, which I will
//  also be responsible for deallocating when its refcount drops to 0


namespace client {


int
InodeManager::AllocInode(ClientSession* session, SuperBlock* sb, int type, Inode** ipp)
{
	return sb->AllocInode(session, type, ipp);
}


int
InodeManager::GetInode(ClientSession* session, SuperBlock* sb, InodeNumber ino, Inode** ipp)
{
	return sb->GetInode(ino, ipp);
}

int
InodeManager::PutInode(ClientSession* session, SuperBlock* sb, Inode* ip)
{
	return 0;
}


}
