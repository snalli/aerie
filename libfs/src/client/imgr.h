#ifndef _INODE_MANAGER_H_DFG143
#define _INODE_MANAGER_H_DFG143

#include <stdint.h>
#include "client/inode.h"
#include "client/imap.h"
#include "client/sb.h"

namespace client {

class InodeManager {
public:

	InodeManager() { }
	int	AllocInode(SuperBlock* sb, int type, Inode** ipp);
	int GetInode(SuperBlock* sb, InodeNumber ino, Inode** ipp);
	int PutInode(SuperBlock* sb, Inode* ip);
	
};

} // namespace client

#endif /* _INODE_MANAGER_H_DFG143 */
