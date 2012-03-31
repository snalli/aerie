#ifndef __STAMNOS_MFS_SERVER_DIRECTORY_INODE_H
#define __STAMNOS_MFS_SERVER_DIRECTORY_INODE_H

#include "ssa/containers/name/container.h"
#include "pxfs/server/inode.h"

namespace server {

class Session; // forward declaration

class DirInode: public InodeT<DirInode> {
public:

	DirInode()
	{ }

	DirInode(InodeNumber ino)
		: InodeT(ino, kDirInode)
	{ }

	int Link(Session* session, const char* name, uint64_t ino);

};

} // namespace server

#endif // __STAMNOS_MFS_SERVER_DIRECTORY_INODE_H
