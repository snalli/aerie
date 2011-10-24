#ifndef _SUPERBLOCK_H_SHD191
#define _SUPERBLOCK_H_SHD191

#include "client/inode.h"

namespace client {

class Session; // forward declaration
class InodeManager;  // forward declaration

class SuperBlock {
public:
	virtual Inode* GetRootInode() = 0;
	//virtual void SetRootInode(Inode* inode) = 0;
	virtual Inode* CreateImmutableInode(int type) = 0;
	virtual int AllocInode(Session* session, int type, Inode** ipp) = 0;
	virtual int GetInode(InodeNumber ino, Inode** ipp) = 0;
	virtual int PutInode(Inode* ip) = 0;
	virtual Inode* WrapInode() = 0;

	virtual void* GetPSuperBlock() = 0;
};


} // namespace client

#endif /* _SUPERBLOCK_H_SHD191 */
