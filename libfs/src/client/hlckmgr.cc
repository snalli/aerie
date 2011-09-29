#include "client/hlckmgr.h"
#include "common/debug.h"
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


HLock::HLock(lock_protocol::LockId lid, HLock* phl)
	: status_(NONE),
	  mode_(lock_protocol::NL),
	  supremum_mode_(lock_protocol::NL),
	  can_retry_(false),
	  used_(false) 
{
	pthread_mutex_init(&mutex_, NULL);
	pthread_cond_init(&status_cv_, NULL);
	pthread_cond_init(&used_cv_, NULL);
}


// assume the thread holds the mutex mutex_
void
HLock::set_status(LockStatus sts)
{
	if (status_ != sts) {
		if (sts == LOCKED) {
			used_ = true;
			pthread_cond_signal(&used_cv_);
		}
		if (sts == NONE) {
			// clear all fields
			used_ = false;
			can_retry_ = false;
		}
		status_ = sts;
		pthread_cond_broadcast(&status_cv_);
	}
}



HLockManager::HLockManager(LockManager* lm, HLockUser* hlu)
	: lm_(lm),
	  hlu_(hlu)
{
	lm_->RegisterLockUser(this);
}


// assumes caller has the mutex mutex_
HLock*
HLockManager::FindOrCreateLockInternal(lock_protocol::LockId lid, 
                                       HLock* plp,
                                       bool create)
{
	HLock* lp;
	
	lp = locks_[lid];
	if (lp == NULL && create) {
		if (plp == NULL) {
			return NULL;
		}
		lp = new HLock(lid, plp);
		locks_[lid] = lp;
	}	
	return lp;
}


/// \fn client::HLockManager::FindOrCreateLock(lock_protocol::LockId lid, lock_protocol::LockId plid)
/// \brief Finds the hierarchical lock identified by lid or if lock lid does 
/// not exist it creates the lock with parent plid.
/// \param lid lock identifier.
/// \param plid parent lock identifier.
/// \return a reference (pointer) to the lock
/// Does no reference counting.
HLock*
HLockManager::FindOrCreateLock(lock_protocol::LockId lid, 
                               lock_protocol::LockId plid)
{
	HLock* hlock;

	pthread_mutex_lock(&mutex_);
	HLock* plp = locks_[plid];
	hlock = FindOrCreateLockInternal(lid, plp, true);
	pthread_mutex_unlock(&mutex_);
	return hlock;
}


inline HLock*
HLockManager::InitLock(lock_protocol::LockId lid, lock_protocol::LockId plid)
{
	//TODO
	//FindOrCreateLockInternal();
}


inline HLock*
HLockManager::InitLock(lock_protocol::LockId lid, HLock* phl)
{
	
}


inline HLock*
HLockManager::InitLock(HLock* hl, HLock* phl)
{

}

bool
HierarchyRule(int mode, HLock* phlock)
{
	(1 << phlock->mode_) & lock_protocol::Mode::hierarchy_rule[mode]

}


// Base lock's mode cannot change without invoking the hierarchical lock manager
// so it is safe to check lock's state without going through the base lock
// manager.
lock_protocol::status
HLockManager::AcquireInternal(pthread_t tid, HLock* hlock, int mode, int flags)
{
	lock_protocol::status r;
	lock_protocol::LockId lid = hlock->lid_;
	HLock*                phlock;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d:%lu] Acquiring hierarchical lock %llu (%s)\n", lm_->id(), 
			tid, lid, lock_protocol::Mode::mode2str(mode).c_str());

	pthread_mutex_lock(&hlock->mutex_);

check_state:
	switch (hlock->status()) {
		case HLock::FREE:
			// great! no one is using the lock
			DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr),
			        "[%d:%lu] lock %llu free locally (base lock mode %s): grant to %lu\n",
			        lm_->id(), tid, lid, 
			        lock_protocol::Mode::mode2str(hlock->lock_->mode(tid)).c_str(), tid);
			
			// check compatibility with current mode.
			if (mode != hlock->mode_) {
				if (lock_protocol::Mode::PartialOrder(mode, hlock->mode_) < 0) {
					// lock can be acquired under current mode. do nothing.
				} else {
					// conversion needed
					if (hlock->lock_) {
						if (lock_protocol::Mode::PartialOrder(mode, hlock->lock_->mode(tid)) < 0) {
							hlock->mode_ = mode;
						} else {
							// must convert base lock
							DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_hlckmgr), 
							        "Must convert base lock\n"); 
						}
					} else {
						phlock = hlock->parent_;
						pthread_mutex_lock(&phlock->mutex_);
						if (phlock->status() == HLock::RELEASING) {
							pthread_mutex_unlock(&phlock->mutex_);
							r = lock_protocol::RETRY;
							goto done;
						}
						if (lock_protocol::Mode::PartialOrder(mode, phlock->mode_) < 0) {
							hlock->mode_ = mode;
						} else {
							// mode is more restrictive than the one allowed by parent.
							// we might need to convert all locks up to the ancestor
							// that is attached to a base lock.
							DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_hlckmgr), 
							        "Must convert ancestors' locks\n"); 
							// TODO: convert; follow hammer approach: publish and release locks 
						}
						pthread_mutex_unlock(&phlock->mutex_);
					}
				}
			}
			hlock->set_status(HLock::LOCKED);
			r = lock_protocol::OK;
			break;
		case HLock::ACQUIRING:
			//TODO
			break;
		case HLock::RELEASING:
			r = lock_protocol::RETRY;
			break;

		case HLock::NONE:
			dbg_log(DBG_INFO, "[%d:%lu] lock %llu not available; acquiring now\n",
			        lm_->id(), tid, lid);
			hlock->set_status(HLock::ACQUIRING);
			
			// algorithm:
			// if we have a parent then try to acquire a lock under the parent.
			// there are two rules we need to meet to be able to get 
			// a lock under a parent:
			// 1) hierarchy rule: there is an intention or recursive lock on 
			//    the parent's globally visible lock.
			//    this rule guarantees we properly lock ancestors before 
			//    acquiring a lock on a node. 
			// 2) supremum rule: my mode is at least or the same severe as 
			//    the mode of the most severe ancestor.
			//    this rule enables us to acquire a local lock without 
			//    invoking the server lock manager. 
			//
			// if we don't meet the hierarchy rule, then we report a hierarchy 
			// violation. 
			// if we don't meet the supremum rule then we acquire a globally
			// visible lock.
			//
			// if we don't have a parent then we need to have a capability, which 
			// we provide to the server to get a globally visible lock.
			
			
			// algorithm:
			// 1) If we have a parent then try to acquire a lock under the parent.
			//    we need to meet the supremum rule:
			//      supremum rule: my mode is at least or the same severe as 
			//      the mode of the most severe ancestor.
			//      this rule enables us to acquire a local lock without 
			//      invoking the server lock manager. 
			//
			//    if we don't meet the supremum rule then we acquire a globally
			//    visible lock.
			//    we need to meet the hierarchy rule:
			//      hierarchy rule: there is an intention or recursive lock on 
			//      the parent's globally visible lock.
			//      this rule guarantees we acquire globally visible locks 
			//      hierarchically. 
			// 
			//    if we don't meet the hierarchy rule, then we report a hierarchy 
			//    violation. 
			//
			// 2) If we don't have a parent then we need to have a capability, which 
			//    we provide to the server to get a globally visible lock.


#if 0
			if (phlock = hlock->parent_) {
				pthread_mutex_lock(&phlock->mutex_);
				
				if (phlock->status() == HLock::RELEASING) {
					pthread_mutex_unlock(&phlock->mutex_);
					r = lock_protocol::RETRY;
					goto done;
				}
			}
			if (lock_protocol::Mode::PartialOrder(mode, phlock->mode_) < 0) {

			
			// FIXME: Acquire may block. drop hlock->mutex_ ?
			r = lm_->Acquire(
			
			if (r == lock_protocol::OK) {
				dbg_log(DBG_INFO, "[%d] thread %lu got lock %llu at seq %d\n",
				        cl2srv_->id(), tid, lid, l->seq_);
				l->global_mode_ = (lock_protocol::mode) mode;
				l->gtque_.Add(ThreadRecord(tid, (lock_protocol::mode) mode));
				l->set_status(Lock::LOCKED);
			}
#endif	
			break;
		default:
			break;

	}


done:
	pthread_mutex_unlock(&hlock->mutex_);
	return r;
}


lock_protocol::status
HLockManager::Acquire(HLock* hlock, int mode, int flags)
{
	lock_protocol::status r;

	r = AcquireInternal(pthread_self(), hlock, mode, flags);
	return r;
}


lock_protocol::status
HLockManager::Acquire(lock_protocol::LockId lid, lock_protocol::LockId plid, 
                      int mode, int flags)
{
	lock_protocol::status r;
	HLock*                hlock;

	hlock = FindOrCreateLock(lid, plid);
	r = AcquireInternal(pthread_self(), hlock, mode, flags);
}


//TODO: Convert method: converts lock into a new mode
//      calls base lock manager if it is attached to a 
// base lock which needs to be converted (e.g upgrade)


lock_protocol::status
HLockManager::ReleaseInternal(pthread_t tid, HLock* hlock)
{
	lock_protocol::status r = lock_protocol::OK;
	lock_protocol::LockId lid = hlock->lid_;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d:%lu] Releasing lock %llu \n", lm_->id(), tid, lid); 
	
	// TODO

	return r;
}


lock_protocol::status
HLockManager::Release(HLock* hlock)
{
	lock_protocol::status r;

	pthread_mutex_lock(&hlock->mutex_);
	r = ReleaseInternal(pthread_self(), hlock);
	pthread_mutex_unlock(&hlock->mutex_);
	return r;
}


lock_protocol::status
HLockManager::Release(lock_protocol::LockId lid)
{
	lock_protocol::status r;
	HLock*                hlock;

	pthread_mutex_lock(&mutex_);
	hlock = FindOrCreateLockInternal(lid, NULL, false);
	pthread_mutex_unlock(&mutex_);
	if (hlock == NULL) {
		return lock_protocol::NOENT;
	}
	pthread_mutex_lock(&hlock->mutex_);
	r = ReleaseInternal(pthread_self(), hlock);
	pthread_mutex_unlock(&hlock->mutex_);
	return r;
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
