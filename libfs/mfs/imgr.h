#ifndef _INODE_MANAGER_H_DFG143
#define _INODE_MANAGER_H_DFG143

#include "mfs/icache.h"
#include <stdint.h>


class InodeManager {
public:

	InodeManager();

	int GetInode(uint64_t ino, bool, Inode** );
	int PutInode(Inode* inode);

private:
	uint64_t    timestamp_;
	InodeCache* icache_;
	
};

#endif /* _INODE_MANAGER_H_DFG143 */
