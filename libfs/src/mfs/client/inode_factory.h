#ifndef __MFS_CLIENT_INODE_FACTORY_H
#define __MFS_CLIENT_INODE_FACTORY_H

#include "common/types.h"
#include "client/backend.h"

namespace mfs {

class InodeFactory {
public:
	static int Open(client::SuperBlock* sb, InodeNumber ino, client::Inode** ipp);
};


} // namespace mfs

#endif // __MFS_CLIENT_INODE_FACTORY_H
