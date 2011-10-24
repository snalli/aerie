#include "mfs/client/dir_inode.h"
#include <stdint.h>
#include "client/inode.h"

using namespace mfs;

int 
DirInodeImmutable::Lookup(client::Session* session, const char* name, client::Inode** inode)
{
	assert(0 && "TODO");
}
	
// This will do an optimistic lookup and put the result in 
// inode. 
//
// If a mutable version of the mutable exists then this function
// will fail and ask you to do the normal slow path lookup
int 
DirInodeImmutable::LookupFast(client::Session* session, const char* name, client::Inode* inode)
{
	assert(0 && "TODO");
}


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


// FIXME: include state to decide whether to do the change locally or remotely
// or have introduce another class DirInodeMutableRemote to do the trick
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
