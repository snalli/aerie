#ifndef __STAMNOS_MFS_CLIENT_SUPERBLOCK_H
#define __STAMNOS_MFS_CLIENT_SUPERBLOCK_H

#include <pthread.h>
#include "common/types.h"
#include "client/backend.h"
#include "mfs/client/dir_inode.h"
#include "mfs/psb.h"

namespace mfs {
namespace client {


//FIXME: SUPERBLOCK
class SuperBlock: public ::client::SuperBlock {
public:
	::client::Inode* RootInode() {
		return root_;
	}

private:

	::client::Inode* root_;
/*	
	SuperBlock(client::Session* session, PSuperBlock<client::Session>* psb)
		: psb_(psb),
		  client::SuperBlock(session)
	{ 
		GetInode(psb->root_, &root_);
	}


	void* GetPSuperBlock() { return (void*) psb_; }

private:
	int LoadInode(InodeNumber ino, client::Inode** ipp);
	int MakeInode(client::Session* session, int type, client::Inode** ipp);
	
	PSuperBlock<client::Session>* psb_;
	//client::InodeMap*             imap_;
*/	
};

} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_SUPERBLOCK_H
