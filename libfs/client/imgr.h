#ifndef _INODE_MANAGER_H_DFG143
#define _INODE_MANAGER_H_DFG143

#include <stdint.h>
#include "client/inode.h"
#include "client/icache.h"

namespace client {

class InodeManager {
public:

	virtual int	AllocInode(int type, Inode** inodep) = 0;
	virtual int GetInode(uint64_t ino, bool, Inode** ) = 0;
	virtual int PutInode(Inode* inode) = 0;
	virtual void SetSuperBlock(SuperBlock* sb) = 0;
protected:
	
};

} // namespace client

#endif /* _INODE_MANAGER_H_DFG143 */
