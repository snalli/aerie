#include "client/inode.h"

namespace client {

Inode::Inode()
	: sb_(NULL),
	  pnode_(NULL),
	  ino_(0)
{ 
	printf("Inode::Inode: %p\n", this);
	pthread_mutex_init(&mutex_, NULL);
}


Inode::Inode(SuperBlock* sb, Pnode* pnode, InodeNumber ino)
	: sb_(sb),
	  pnode_(pnode),
	  ino_(ino)
{
	printf("Inode::Inode: %p\n", this);
	pthread_mutex_init(&mutex_, NULL);
}


//FIXME: when creating the inode, inode manager assigns a public lock if 
// needed. if there is a public lock, then we acquire it otherwise we acquire the
// private lock
int
Inode::Lock(lock_protocol::Mode mode)
{
	if (ino_) {
		return global_hlckmgr->Acquire(ino_, mode, 0);
	}
	return 0;
}


int
Inode::Lock(Inode* parent_inode, lock_protocol::Mode mode)
{
	InodeNumber pino = parent_inode->ino_;
	printf("LOCK: inode=%p, parent_inode=%p, pino=%lu, ino_=%lu\n", this, parent_inode, pino, ino_);
	if (ino_) {
		if (pino) {
			// root inode of the file system. 
			// FIXME: need to pass a capability
			return global_hlckmgr->Acquire(ino_, pino, mode, 0);
		} else {
			return global_hlckmgr->Acquire(ino_, mode, 0);
		}
	}
	return 0;
}


int
Inode::Unlock()
{
	if (ino_) {
		return global_hlckmgr->Release(ino_);
	}
	return 0;
}


int 
Inode::Get() 
{ 
	pthread_mutex_lock(&mutex_);
	printf("Inode(%p)::Get %d -> %d\n", this, refcnt_, refcnt_ + 1);
	refcnt_++; 
	pthread_mutex_unlock(&mutex_);
	return 0; 
}


int 
Inode::Put() 
{ 
	pthread_mutex_lock(&mutex_);
	printf("Inode(%p)::Put %d -> %d\n", this, refcnt_, refcnt_ - 1);
	assert(refcnt_>0); 
	refcnt_--; 
	pthread_mutex_unlock(&mutex_);
	return 0; 
}

} // namespace client
