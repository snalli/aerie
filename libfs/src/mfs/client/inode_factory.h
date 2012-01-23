#ifndef __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
#define __STAMNOS_MFS_CLIENT_INODE_FACTORY_H

#include "common/types.h"
#include "client/backend.h"
#include "client/inode_factory.h"
#include "common/const.h"

namespace mfs {
namespace client {

class InodeFactory: public ::client::InodeFactory {
public:
	int Make(::client::Session* session, int type, ::client::Inode** ipp);
	int Load(::client::Session* session, dpo::common::ObjectId oid, ::client::Inode** ipp);
};


} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
