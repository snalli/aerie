#ifndef __STAMNOS_FS_CLIENT_SUPERBLOCK_H
#define __STAMNOS_FS_CLIENT_SUPERBLOCK_H

#include "common/types.h"
#include "pxfs/client/inode.h"
#include "ssa/main/client/hlckmgr.h"
#include "ssa/main/common/obj.h"

/**
 *
 * The SuperBlock provides an indirection layer to the inodes of the 
 * filesystem and plays the role of an inode manager.
 *
 * We need indirection because a client cannot directly write to persistent 
 * inodes. Instead we wrap a persistent inode into a local/private one that
 * performs writes using copy-on-write (either logical or physical). 
 * 
 * The SuperBlock is responsible for keeping track the index of inode number 
 * to (volatile) inode. 
 *
 */

namespace client {

class Session; // forward declaration

// Abstract class representing a file-system specific superblock
class SuperBlock {
public:
	virtual client::Inode* RootInode() = 0;	
	virtual ssa::common::ObjectId oid() = 0;
};


} // namespace client

#endif // __STAMNOS_FS_CLIENT_SUPERBLOCK_H
