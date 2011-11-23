#include "mfs/client/sb.h"
#include <pthread.h>
#include "client/backend.h"
#include "mfs/psb.h"
#include "mfs/client/dir_inode.h"
#include "mfs/client/inode_factory.h"

namespace mfs {

// TODO:
// the superblock should only provide file-system functionality, that is construction of the 
// inode. it should behave as a factory. that is, re-route the request to the local factory. 
// Another approach would be for each file-system to define and register object factories
// with the generic superblock (and storage manager). 
// The bottom line is that we need some form of polymorphism, either at the superblock or at the factory.

int
SuperBlock::MakeInode(client::Session* session, int type, client::Inode** ipp)
{
	client::Inode*              ip;
	DirPnode<client::Session>*  dpnode;
	
	printf("mfs::SuperBlock::AllocInode\n");
	
	switch (type) {
		case client::type::kFileInode:
			//ip = new 	
			break;
		case client::type::kDirInode:
			dpnode = new(session) DirPnode<client::Session>;
			ip = new DirInodeMutable(this, dpnode);
			break;
	}

	*ipp = ip;
	return 0;
}


// creates an inode that wraps the persistent inode identified by number ino
int
SuperBlock::LoadInode(InodeNumber ino, client::Inode** ipp)
{
	int            ret;
	client::Inode* ip;

	assert((ret = InodeFactory::Open(this, ino, &ip)) == 0);
	*ipp = ip;
	return ret;
}

} // namespace mfs
