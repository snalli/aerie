#include "client/hlckmgr.h"
#include "common/debug.h"
#include "common/bitmap.h"
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


// Notes:
// 1) Cannot change the state of a base lock attached to a hierarchical lock 
//    without acquring the lock protecting the hierarchical lock.
// 2) Hierarchical lock acts as mutex lock between threads, that is multiple 
//    threads cannot acquire the lock even if the locked is held at a mode
//    permitting multiple owners (e.g. SL). 


HLock::HLock(lock_protocol::LockId lid, HLock* phl)
	: status_(NONE),
	  mode_(lock_protocol::Mode(lock_protocol::Mode::NL)),
	  ancestor_recursive_mode_(lock_protocol::Mode(lock_protocol::Mode::NL)),
	  can_retry_(false),
	  used_(false),
	  lid_(lid),
	  parent_(phl),
	  lock_(NULL)
{
	pthread_mutex_init(&mutex_, NULL);
	pthread_cond_init(&status_cv_, NULL);
	pthread_cond_init(&used_cv_, NULL);
	children_.set_empty_key(NULL);
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
	pthread_mutex_init(&mutex_, NULL);
	locks_.set_empty_key(-1);
}


HLock*
HLockManager::FindLockInternal(lock_protocol::LockId lid, HLock* plp)
{
	HLock* lp;
	
	// if lid does not exist then the initial value of the slot is 0 (NULL)
	lp = locks_[lid];
	return lp;
}


// assumes caller has the mutex mutex_
// if parent lock plp is NULL then we create a lock without a parent
// which is useful for root locks
HLock*
HLockManager::FindOrCreateLockInternal(lock_protocol::LockId lid, HLock* plp)
{
	HLock* lp;
	
	lp = locks_[lid];
	if (lp == NULL) {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
		        "[%d] Creating hierarchical lock %llu with parent %llu\n", lm_->id(), 
		        lid, plp != 0 ? plp->lid_:0);
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
	hlock = FindOrCreateLockInternal(lid, plp);
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
	//(1 << phlock->mode_) & lock_protocol::Mode::hierarchy_rule[mode]

}


//BreakLock
int BreakLock()
{

}


// Base lock's mode cannot change without invoking the hierarchical lock manager
// so it is safe to check lock's state without going through the base lock
// manager.
//
// locking protocol:
// 1) If we have a parent then try to acquire a local lock under the 
//    parent. 
//    we need to meet the recursive rule:
//      recursive rule: some ancestor has a recursive lock 
//      this rule enables us to acquire a local lock without 
//      invoking the server lock manager. 
//
//    if we fail the recursive rule then we acquire a globally
//    visible lock.
//    we need to meet the hierarchy rule:
//      hierarchy rule: there is an intention or recursive lock on 
//      the parent's globally visible lock that is compatible.
//      this rule guarantees we acquire globally visible locks 
//      hierarchically. 
//    e.g if SR and need IXSL, we violate the hierarchy rule
//
//    if we don't meet the hierarchy rule, then we report a hierarchy 
//    violation. 
//
// 2) If we don't have a parent then we need to have a capability, which 
//    we provide to the server to get a globally visible lock.


lock_protocol::status
HLockManager::AcquireInternal(pthread_t tid, HLock* hlock, 
                              lock_protocol::Mode mode, int flags)
{
	lock_protocol::status r;
	lock_protocol::LockId lid = hlock->lid_;
	lock_protocol::Mode   mode_granted;
	HLock*                phlock;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d:%lu] Acquiring hierarchical lock %llu (%s)\n", lm_->id(), 
			tid, lid, mode.String().c_str());
	
	if (!hlock) {
		r = lock_protocol::NOENT;
		return r;
	}

	pthread_mutex_lock(&hlock->mutex_);

check_state:
	switch (hlock->status()) {
		case HLock::FREE:
			// great! no one is using the lock
			if (hlock->lock_) {
				DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr),
			            "[%d:%lu] hierarchical lock %llu free locally (base lock mode %s): "
					    "grant to thread %lu\n",
			            lm_->id(), tid, lid, 
			            hlock->lock_->global_mode_.String().c_str(), tid);
				if (mode < hlock->mode_) {
					hlock->owner_ = tid;
					hlock->set_status(HLock::LOCKED);
					r = lock_protocol::OK;
					break;
				} else {
					if (mode < hlock->lock_->global_mode_) {
						hlock->mode_ = lock_protocol::Mode::Supremum(hlock->mode_, mode);
						assert((hlock->mode_ == hlock->lock_->global_mode_) ||
						       (hlock->mode_ < hlock->lock_->global_mode_));
						hlock->owner_ = tid;	
						hlock->set_status(HLock::LOCKED);
						r = lock_protocol::OK;
					} else {
						// must convert base lock
						DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_hlckmgr), 
								"Must convert base lock\n");
					}
				}
			} else {
				if (phlock = hlock->parent_) {
					if (lock_protocol::Mode::AbidesRecursiveRule(mode, 
						   hlock->ancestor_recursive_mode_))
					{
						hlock->mode_ = lock_protocol::Mode::Supremum(hlock->mode_, mode);
						assert(lock_protocol::Mode::AbidesRecursiveRule(hlock->mode_, hlock->ancestor_recursive_mode_));
						hlock->owner_ = tid;
						hlock->set_status(HLock::LOCKED);
						r = lock_protocol::OK;
					}
				} else {
					DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_hlckmgr), 
							"No base lock and no parent\n"); 
				}
			}
			break;
		case HLock::ACQUIRING:
			// There is on-going lock acquisition; sit here and wait
			// its completion
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
			        "[%d:%lu] lck-%llu: another thread in acquisition\n",
			        lm_->id(), tid, lid);
			if (flags & Lock::FLG_NOBLK) {
				r = lock_protocol::RETRY;
				break;
			}
			while (hlock->status() == HLock::ACQUIRING) {
				pthread_cond_wait(&hlock->status_cv_, &hlock->mutex_);
			}
			goto check_state;	// Status changed. Re-check.
		case HLock::RELEASING:
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
			        "[%d:%lu] hierarchical lock manager is releasing lock %llu\n",
			        lm_->id(), tid, lid);
			while (hlock->status() == HLock::RELEASING) {
				pthread_cond_wait(&hlock->status_cv_, &hlock->mutex_);
			}
			goto check_state;	// Status changed. Re-check.
		case HLock::LOCKED:
			if (hlock->owner_ == tid) {
				// current thread has already obtained the lock
				DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
				        "[%d:%lu] current thread already got lck %llu\n",
				        lm_->id(), tid, lid);
				r = lock_protocol::OK;
			} else {
				if (flags & Lock::FLG_NOBLK) {
					r = lock_protocol::RETRY;
					break;
				}
				while (hlock->status() == HLock::LOCKED) {
					pthread_cond_wait(&hlock->status_cv_, &hlock->mutex_);
				}
				goto check_state;	// Status changed. Re-check.
			}
			break;
		case HLock::NONE:
			dbg_log(DBG_INFO, "[%d:%lu] lock %llu not available; acquiring now\n",
			        lm_->id(), tid, lid);
			
			hlock->set_status(HLock::ACQUIRING);

			if (phlock = hlock->parent_) {
				pthread_mutex_lock(&phlock->mutex_);
				// check we own the parent lock
				// FIXME: there is still a TOCTTOU race: another client grabbed the lock
				// moved us under a new parent, and then some other fellow thread locks 
				// the old parent. we fail finding the parent has changed. UNIX provides
				// these semantics but not sure whether these semantics are safe for us.
				// SOLUTION: use version numbers on locks, or have a notification mechanism
				// that cancels on going lock acquisitions on a release and restarts any
				// lookups, or before acquiring a lock register the node as the child of 
				// the parent, and the child still checks the node is a parent. 
				// the registration is done by lookup before releasing the lock on the 
				// parent 
				
				// FIXME: if we enforce that a user must perform spider-locking then we 
				// should not allow HLock::FREE
				if (!(phlock->status() == HLock::LOCKED || 
				      phlock->status() == HLock::FREE))
				{
					pthread_mutex_unlock(&phlock->mutex_);
					r = lock_protocol::RETRY;
					goto done;
				}
				if (lock_protocol::Mode::AbidesRecursiveRule(mode, phlock->ancestor_recursive_mode_))
				{
					phlock->AddChild(hlock);
					hlock->ancestor_recursive_mode_ = phlock->ancestor_recursive_mode_;
					hlock->owner_ = tid;
					hlock->set_status(HLock::LOCKED);
					r = lock_protocol::OK;
				} else {
					// failed the recursive rule. acquire a globally visible lock
					//TODO: test hierarchy rule
					
					// FIXME: Acquire may block. drop hlock->mutex_ ?
					// what can go wrong? i haven't attached myself to the parent 
					// so nobody can see me
					//pthread_mutex_unlock(&hlock->mutex_);
					
					// FIXME: we release parent mutex so parent lock might have been 
					// dropped till we get the mutex and check again.
					// same TOCTTOU race as above
					lock_protocol::Mode::Set mode_set;
					mode_set.Insert(mode);
					mode_set.Insert(mode.LeastRecursiveMode());
					pthread_mutex_unlock(&phlock->mutex_);
					r = lm_->Acquire(lid, mode_set, flags, mode_granted);
					
					pthread_mutex_lock(&phlock->mutex_);
					if (!(phlock->status() == HLock::LOCKED || 
						  phlock->status() == HLock::FREE))
					{
						pthread_mutex_unlock(&phlock->mutex_);
						r = lock_protocol::RETRY;
						goto done;
					}

					if (r == lock_protocol::OK) {
						dbg_log(DBG_INFO, "[%d:%lu] thread %lu got lock %llu\n",
								lm_->id(), tid, tid, lid);
						if (mode_granted == lock_protocol::Mode(lock_protocol::Mode::XR) || 
						    mode_granted == lock_protocol::Mode(lock_protocol::Mode::SR)) 
						{
							hlock->ancestor_recursive_mode_ = mode_granted;
						} else {
							hlock->ancestor_recursive_mode_ = lock_protocol::Mode::NL;
						}
						phlock->AddChild(hlock);
						hlock->lock_ = lm_->FindLock(lid);
						hlock->lock_->payload_ = static_cast<void *>(hlock);
						hlock->mode_ = mode;
						hlock->owner_ = tid;
						hlock->set_status(HLock::LOCKED);
						r = lock_protocol::OK;
					}
				}

				pthread_mutex_unlock(&phlock->mutex_);
			} else {
				// TODO: no parent. need to have a capability given from the server
				lock_protocol::Mode::Set mode_set;
				mode_set.Insert(mode);
				mode_set.Insert(mode.LeastRecursiveMode());
				r = lm_->Acquire(lid, mode_set, flags, mode_granted);
				if (r == lock_protocol::OK) {
					dbg_log(DBG_INFO, "[%d:%lu] thread %lu got lock %llu\n",
							lm_->id(), tid, tid, lid);
					if (mode_granted == lock_protocol::Mode(lock_protocol::Mode::XR) || 
						    mode_granted == lock_protocol::Mode(lock_protocol::Mode::SR)) 
					{
						hlock->ancestor_recursive_mode_ = mode_granted;
					} else {
						hlock->ancestor_recursive_mode_ = lock_protocol::Mode::NL;
					}
					hlock->lock_ = lm_->FindLock(lid);
					hlock->lock_->payload_ = static_cast<void *>(hlock);
					hlock->mode_ = mode;
					hlock->owner_ = tid;
					hlock->set_status(HLock::LOCKED);
					r = lock_protocol::OK;
				}
			}
			break;
		default:
			break;

	}


done:
	pthread_mutex_unlock(&hlock->mutex_);
	return r;
}


lock_protocol::status
HLockManager::Acquire(HLock* hlock, lock_protocol::Mode mode, int flags)
{
	lock_protocol::status r;

	r = AcquireInternal(pthread_self(), hlock, mode, flags);
	return r;
}


lock_protocol::status
HLockManager::Acquire(lock_protocol::LockId lid, lock_protocol::LockId plid, 
                      lock_protocol::Mode mode, int flags)
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
	
	if (hlock->owner_ == tid) {
		hlock->set_status(HLock::FREE);
	} else {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d:%lu] thread %lu is not holder of lck %llu\n",
		        lm_->id(), tid, tid, lid);
		r = lock_protocol::NOENT;
	}

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
	hlock = FindLockInternal(lid, NULL);
	pthread_mutex_unlock(&mutex_);
	if (hlock == NULL) {
		return lock_protocol::NOENT;
	}
	pthread_mutex_lock(&hlock->mutex_);
	r = ReleaseInternal(pthread_self(), hlock);
	pthread_mutex_unlock(&hlock->mutex_);
	return r;
}

// revocations must be serialized. for example, consider /X/Y/Z. Revoking
// Y cannot be done concurrently with revoking X. 
int 
HLockManager::Revoke(Lock* lp, lock_protocol::Mode mode)
{
	// if can downgrade a lock by silently upgrading its children 
	// then you don't need to publish.
	// CHALLENGE: if you come here via a synchronous call from the base lock manager
	// then you can't block when trying to upgrade the children's locks. 
	// you either need to respond to the caller immediately and acquire locks 
	// asynchronously in a different thread (i.e. context switch or user level thread package)
	// or need to try to acquire locks without blocking. the lock protocol
	// should guarantee whether you can acquire locks without blocking. for example,
	// if you already have a recursive lock then you know that you can acquire a recursive 
	// lock on the children as well. so there are several children upgrades which
	// you know can happen. 
	
	HLock* hlock;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d] Revoking lock %llu to %s\n", lm_->id(), lp->lid_, mode.String().c_str()); 
	
	hlock = static_cast<HLock*>(lp->payload_);

	pthread_mutex_lock(&hlock->mutex_);
	
check_state:	
	switch(hlock->status()) {
		case HLock::FREE:
			hlock->set_status(HLock::RELEASING);
			break;
		case HLock::ACQUIRING:
		case HLock::LOCKED:
			pthread_cond_wait(&hlock->status_cv_, &hlock->mutex_);
			goto check_state;	// Status changed. Re-check.
		case HLock::RELEASING:
			assert(0); // who is releasing it?
	}

	pthread_mutex_unlock(&hlock->mutex_);

	if (mode == hlock->mode_) {
		lm_->Convert(lp, mode, true);
		// what if convert fails? 
	} else {
		lm_->Release(lp);
		// what if release fails?
	}
	
	return 0;
}


} // namespace client
