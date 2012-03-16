#ifndef __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
#define __STAMNOS_MFS_CLIENT_INODE_FACTORY_H

#include "common/types.h"
//#include "client/backend.h"
#include "pxfs/client/inode_factory.h"
#include "common/const.h"

namespace mfs {
namespace client {

class InodeFactory: public ::client::InodeFactory {
public:
	int Make(::client::Session* session, int type, ::client::Inode** ipp);
	int Load(::client::Session* session, ssa::common::ObjectId oid, ::client::Inode** ipp);

	/* non-polymorphic functions */
	static int MakeInode(::client::Session* session, int type, ::client::Inode** ipp);
	static int LoadInode(::client::Session* session, ssa::common::ObjectId oid, ::client::Inode** ipp);
	static int LoadDirInode(::client::Session* session, ssa::common::ObjectId oid, ::client::Inode** ipp);
	static int LoadFileInode(::client::Session* session, ssa::common::ObjectId oid, ::client::Inode** ipp);
	static int MakeDirInode(::client::Session* session, ::client::Inode** ipp);
	static int MakeFileInode(::client::Session* session, ::client::Inode** ipp);

private:
	static pthread_mutex_t mutex_;
};


} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
