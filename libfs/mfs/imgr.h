#ifndef _MFS_INODE_MANAGER_H_DFG143
#define _MFS_INODE_MANAGER_H_DFG143

#include <stdint.h>
#include "client/backend.h"

namespace mfs {

class InodeManager: public client::InodeManager {
public:

	InodeManager(client::SuperBlock* sb, client::StorageManager* smgr);

	int	AllocInode(int type, client::Inode** inodep);
	int GetInode(uint64_t ino, bool, client::Inode** );
	int PutInode(client::Inode* inode);

	void SetSuperBlock(client::SuperBlock* sb) { sb_ = sb; }

private:
	uint64_t                timestamp_;
	InodeCache*             icache_;
	client::SuperBlock*     sb_;
	client::StorageManager* smgr_;
};

} // namespace mfs

#endif /* _INODE_MANAGER_H_DFG143 */
