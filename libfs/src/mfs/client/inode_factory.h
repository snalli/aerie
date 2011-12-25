#ifndef __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
#define __STAMNOS_MFS_CLIENT_INODE_FACTORY_H

#include "common/types.h"
#include "client/backend.h"

namespace mfs {
namespace client {

class InodeFactory {
public:
	static int Open(client::SuperBlock* sb, InodeNumber ino, client::Inode** ipp);
};


} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_INODE_FACTORY_H
