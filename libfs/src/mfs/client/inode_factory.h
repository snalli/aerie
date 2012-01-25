#ifndef __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
#define __STAMNOS_MFS_CLIENT_INODE_FACTORY_H

#include "common/types.h"
#include "client/backend.h"
#include "client/inode_factory.h"
#include "common/const.h"

namespace mfs {
namespace client {

class SuperBlockFactory; // forward declaration

class InodeFactory: public ::client::InodeFactory {
friend class SuperBlockFactory;
public:
	int Make(::client::Session* session, int type, ::client::Inode** ipp);
	int Load(::client::Session* session, dpo::common::ObjectId oid, ::client::Inode** ipp);

private:
	static int LoadDirInode(::client::Session* session, dpo::common::ObjectId oid, ::client::Inode** ipp);
	static int LoadFileInode(::client::Session* session, dpo::common::ObjectId oid, ::client::Inode** ipp);
	static int MakeDirInode(::client::Session* session, ::client::Inode** ipp);
	static int MakeFileInode(::client::Session* session, ::client::Inode** ipp);

	static pthread_mutex_t mutex_;
};


} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
