#ifndef _MFS_CLIENT_SUPERBLOCK_H_KAJ911
#define _MFS_CLIENT_SUPERBLOCK_H_KAJ911

#include "client/backend.h"
#include <pthread.h>
#include "mfs/client/dir_inode.h"
#include "mfs/psb.h"

namespace mfs {

class SuperBlock: public client::SuperBlock {
public:

	SuperBlock(client::Session* session, PSuperBlock<client::Session>* psb)
		: psb_(psb),
		  client::SuperBlock(session)
	{ 
		LoadInode(psb->root_, &root_);
	}


	void* GetPSuperBlock() { return (void*) psb_; }

private:
	int LoadInode(client::InodeNumber ino, client::Inode** ipp);
	int MakeInode(client::Session* session, int type, client::Inode** ipp);
	
	PSuperBlock<client::Session>* psb_;
	//client::InodeMap*             imap_;
	
};


} // namespace mfs

#endif /* _MFS_CLIENT_SUPERBLOCK_H_KAJ911 */
