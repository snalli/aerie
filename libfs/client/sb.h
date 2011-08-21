#ifndef _SUPERBLOCK_H_SHD191
#define _SUPERBLOCK_H_SHD191

#include "client/inode.h"

namespace client {

class InodeManager;  // forward declaration

class SuperBlock {
public:
	virtual Inode* GetRootInode() = 0;
	//virtual void SetRootInode(Inode* inode) = 0;
	virtual Inode* CreateImmutableInode(int t) = 0;
	virtual Inode* CreateInode(int t) = 0;
	virtual Inode* WrapInode() = 0;

	virtual void* GetPSuperBlock() = 0;
	virtual InodeManager* get_imgr() = 0;
protected:
	InodeManager* imgr_;
};


} // namespace client

#endif /* _SUPERBLOCK_H_SHD191 */
