#ifndef __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
#define __STAMNOS_MFS_CLIENT_INODE_FACTORY_H

#include "pxfs/common/types.h"
//#include "client/backend.h"
#include "pxfs/client/inode_factory.h"
#include "pxfs/common/const.h"

namespace mfs {
namespace client {

class InodeFactory: public ::client::InodeFactory {
public:
	int Make(::client::Session* session, int type, ::client::Inode** ipp);
	int Load(::client::Session* session, osd::common::ObjectId oid, ::client::Inode** ipp);
	int Destroy(::client::Session* session, ::client::Inode* ip);

	/* non-polymorphic functions */
	static int MakeInode(::client::Session* session, int type, ::client::Inode** ipp);
	static int LoadInode(::client::Session* session, osd::common::ObjectId oid, ::client::Inode** ipp);
	static int DestroyInode(::client::Session* session, ::client::Inode* ip);
	static int LoadDirInode(::client::Session* session, osd::common::ObjectId oid, ::client::Inode** ipp);
	static int LoadFileInode(::client::Session* session, osd::common::ObjectId oid, ::client::Inode** ipp);
	static int MakeDirInode(::client::Session* session, ::client::Inode** ipp);
	static int MakeFileInode(::client::Session* session, ::client::Inode** ipp);
	static int DestroyFileInode(::client::Session* session, ::client::Inode* ip);

private:
	static pthread_mutex_t mutex_;
};


} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
