#include "client/hlckmgr.h"
#include "client/lckmgr.h"


namespace client {

// Lock Protocol:
//
// 
// read-only file:
//
// need to locally lock each inode along the path at level SL. 
// if the local lock has no parent lock then you need to 
// acquire a base SR lock on the inode.
// 
// read-write file:
//
// need to locally lock each inode except the file inode at IXSL
// if the local lock has no parent lock then you need to 
// acquire a base XR lock on the inode. 
// locally lock the file inode at XL
//
//
// Downgrade protocol:
//
// SR --> SL 
// upgrade each child's local SL lock to a base SR lock 
// 
// OR? 
//
// SR --> IXSL
// we need this downgrade if we want to prevent someone from 
// getting the base lock at SR. I don't think we need this 
// because the children lock at SR to allow this downgrade 
// to happen.
//
// XR --> IXSL
// upgrade each child's IXSL lock to a base XR lock
//
// all other downgrades require releasing the subtree of locks
//   e.g SR --> NL: release subtree
//
// Upgrade protocol:
//
// if cannot upgrade base lock then release all locks
//
//
// Enhancements:
// - we probably need a name lock to fine grain synchronize accesses 
//   in the same inode


HLock::HLock(lock_protocol::LockId lid)
{
	// this creates a root lock 
	// need to allocate Lock lid

}


HLock::HLock(lock_protocol::LockId lid, HLock* phl)
{
	// this creates a child lock under parent HLock phl
	// don't need to allocate Lock lid
}


HLockManager::HLockManager(LockManager* lm, HLockUser* hlu)
	: lm_(lm),
	  hlu_(hlu)
{
	lm_->RegisterLockUser(this);
}


int 
HLockManager::Revoke(Lock* lp)
{
	// if can downgrade a lock by silently upgrading its children 
	// then you don't need to publish.
	// CHALLENGE: if you come here via a synchronous call by the base lock manager
	// then you can't block when trying to upgrade the children's locks. 
	// you either need to respond to the caller immediately and acquire locks 
	// asynchronously in a different thread (i.e. context switch or user level thread package)
	// or need to try to acquire locks without blocking. the lock protocol
	// should guarantee whether you can acquire locks without blocking. for example,
	// if you already have a recursive lock then you know that you can acquire a recursive 
	// lock on the children as well. so there are several children upgrades which
	// you know can happen. 

}





} // namespace client
