#include "ssa/main/client/hlckmgr.h"
#include "common/bitmap.h"
#include "bcs/bcs.h"
#include "ssa/main/client/lckmgr.h"


namespace ssa {
namespace cc {
namespace client {

/** 
 * POLICIES
 * 
 * Downgrade:
 *
 * SR --> SL 
 * upgrade each child's local SL lock to a base SR lock 
 * 
 * OR? 
 *
 * SR --> IXSL
 * we need this downgrade if we want to prevent someone from 
 * getting the base lock at SR. I don't think we need this 
 * because the children lock at SR to allow this downgrade 
 * to happen.
 *
 * XR --> IXSL
 * upgrade each child's IXSL lock to a base XR lock
 *
 * all other downgrades require releasing the subtree of locks
 *   e.g SR --> NL: release subtree
 *
 * Upgrade:
 *
 * if cannot upgrade base lock then release all locks
 *
 */

/**
 * Invariants 
 *
 * 1) Base lock's mode cannot change without invoking the hierarchical lock 
 * manager so it's safe to check lock's state without going through the 
 * base lock manager.
 *    (Cannot change the state of a base lock attached to a hierarchical lock 
 *     without acquring the lock protecting the hierarchical lock.)
 *
 *
 * Locking protocol:
 * 
 * Recursive Rule: some ancestor has a public recursive lock. 
 *  - this rule enables us to acquire a private lock without invoking the server 
 *    lock manager. 
 *
 * Hierarchy Rule: there is an intention or recursive lock on the parent's
 * lock that protects us. 
 *  - this rule guarantees we acquire locks hierarchically. 
 *
 *
 * 1) If we have a parent then try to acquire a private lock under the 
 *    parent. 
 *    we need to meet the recursive rule:
 *
 *    if we fail the recursive rule then we acquire a public lock.
 *    we need to meet the hierarchy rule:
 *    e.g if SR and need IXSL, we violate the hierarchy rule
 *
 *    if we don't meet the hierarchy rule, then we report a hierarchy 
 *    violation. 
 *
 * 2) If we don't have a parent then we need to have a capability, which 
 *    we provide to the server to get a globally visible lock.
 *
 *
 * Notes:
 * 1) Hierarchical lock acts as mutex lock between threads, that is multiple 
 *    threads cannot acquire the lock even if the locked is held at a mode
 *    permitting multiple owners (e.g. SL). 
 * 
 *
 * TOCTTOU RACE:
 *
 * The locking protocol does not protect against TOCTTOU races sush as 
 * when we check we are under a parent and then someone moves us under 
 * a new parent. The user is responsible for ensuring such races don't 
 * happen by doing lock-coupling of parent and child.
 *
 *
 * States:
 *
 * CONVERTING: intermediate state. 
 * prevents others from modifying lock metadata such as public lock, 
 * mode, children set. 
 * enables a high priority task to abort a low priority task.
 */

HLock::HLock(LockId lid)
	: status_(NONE),
	  mode_(lock_protocol::Mode(lock_protocol::Mode::NL)),
	  ancestor_recursive_mode_(lock_protocol::Mode(lock_protocol::Mode::NL)),
	  can_retry_(false),
	  used_(false),
	  lid_(lid),
	  parent_(NULL),
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


int
HLock::BeginConverting(bool lock) 
{
	if (lock) {
		pthread_mutex_lock(&mutex_);
	}
	while (!(status_ == HLock::FREE || 
	         status_ == HLock::LOCKED)) 
	{
		pthread_cond_wait(&status_cv_, &mutex_);
	}
	if (status_ == HLock::LOCKED) {
		set_status(HLock::LOCKED_CONVERTING);
	} else {
		set_status(HLock::CONVERTING);
	}
	if (lock) {
		pthread_mutex_unlock(&mutex_);
	}
	return 0;
}


int
HLock::EndConverting(bool lock) 
{
	if (lock) {
		pthread_mutex_lock(&mutex_);
	}
	if (status_ == HLock::LOCKED_CONVERTING) {
		set_status(HLock::LOCKED);
	} else {
		set_status(HLock::FREE);
	}
	if (lock) {
		pthread_mutex_unlock(&mutex_);
	}
	return 0;
}


// \brief waits the lock to reach status old_status and then changes lock
// to new_status
int
HLock::ChangeStatus(LockStatus old_sts, LockStatus new_sts) 
{
	pthread_mutex_lock(&mutex_);
	while (status_ != old_sts) {
		pthread_cond_wait(&status_cv_, &mutex_);
	}
	set_status(new_sts);
	pthread_mutex_unlock(&mutex_);
	return 0;
}


// \brief waits the lock to reach status old_status
//
// assumes mutex lock is help
int
HLock::WaitStatus2(LockStatus old_sts1, LockStatus old_sts2) 
{
	while (!(status_ == old_sts1 || status_ == old_sts2)) {
		pthread_cond_wait(&status_cv_, &mutex_);
	}
	return 0;
}

// \brief waits the lock to reach status old_status
//
// assumes mutex lock is help
int
HLock::WaitStatus(LockStatus old_sts) 
{
	while (!(status_ == old_sts)) {
		pthread_cond_wait(&status_cv_, &mutex_);
	}
	return 0;
}


HLockManager::HLockManager(LockManager* lm)
	: lm_(lm),
	  hcb_(NULL),
	  status_(NONE)
{
	lm_->RegisterLockRevoke(HLock::TypeId, this);
	pthread_mutex_init(&mutex_, NULL);
	pthread_cond_init(&status_cv_, NULL);
	locks_.set_empty_key(LockId(0, 0));
	locks_.set_deleted_key(LockId(-1, 0));
}


HLockManager::~HLockManager()
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d] Shutting down Hierarchical Lock Manager\n", id());

	lm_->UnregisterLockRevoke(HLock::TypeId);	

	// make sure no other thread is running inside this object 
	// (e.g. a thread making a callback to Revoke)
	pthread_mutex_lock(&mutex_);
	while (status_ != NONE) {
		pthread_cond_wait(&status_cv_, &mutex_);
	}
	status_ = SHUTTING_DOWN;
	pthread_mutex_unlock(&mutex_);


	// release any base locks held by the hierarchical locks
	LockMap::iterator itr;
	for (itr = locks_.begin(); itr != locks_.end(); ++itr) {
		// we don't need to synchronously release the lock as the 
		// base lock manager's destructor should finally release the locks
		if (itr->second->lock_) {
			assert(lm_->Release(itr->second->lock_, false) == lock_protocol::OK); 
		}
	}

	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d] Shutting down Hierarchical Lock Manager: DONE\n", id());
}


HLock*
HLockManager::FindLockInternal(LockId lid)
{
	HLock* lp;
	
	// if lid does not exist then the initial value of the slot is 0 (NULL)
	lp = locks_[lid];
	return lp;
}


// assumes caller holds the lock mutex_
// if parent lock plp is NULL then we create a lock without a parent
// which is useful for root locks
HLock*
HLockManager::FindOrCreateLockInternal(LockId lid)
{
	HLock* lp;
	
	lp = locks_[lid];
	if (lp == NULL) {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
		        "[%d] Creating hierarchical lock %s\n", id(), lid.c_str());
		lp = new HLock(lid);
		locks_[lid] = lp;
	}	
	return lp;
}


/**
 * \fn client::HLockManager::FindOrCreateLock(lock_protocol::LockId lid, lock_protocol::LockId plid)
 * \brief Finds the hierarchical lock identified by lid or if lock lid does 
 * not exist it creates the lock with parent plid.
 * \param lid lock identifier.
 * \param plid parent lock identifier.
 * \return a reference (pointer) to the lock
 * Does no reference counting.
 */
HLock*
HLockManager::FindOrCreateLock(LockId lid)
{
	HLock* hlock;

	pthread_mutex_lock(&mutex_);
	hlock = FindOrCreateLockInternal(lid);
	pthread_mutex_unlock(&mutex_);
	return hlock;
}


/// \brief Acquires and attaches a base lock on a hierarchical lock using
//  a capability
/// 
/// Assumes:
///   caller holds hlock's mutex or has the lock set at CONVERTING state
///
lock_protocol::status
HLockManager::AttachPublicLockCapability(HLock* hlock, lock_protocol::Mode mode, int flags)
{
	Lock*                    lock;
	LockId                   lid = hlock->lid_;
	lock_protocol::Mode::Set mode_set;
	lock_protocol::Mode      mode_granted;
	int                      r;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d] Attaching public lock %s to hierarchical lock %s (%s) using capability\n", id(), 
			lid.c_str(), lid.c_str(), mode.String().c_str());

	mode_set.Insert(mode);
	mode_set.Insert(mode.LeastRecursiveMode());

	r = lm_->Acquire(lid, mode_set, lock_protocol::FLG_CAPABILITY, mode_granted);

	if (r == lock_protocol::OK) {
		if (mode_granted == lock_protocol::Mode(lock_protocol::Mode::XR) || 
			mode_granted == lock_protocol::Mode(lock_protocol::Mode::SR)) 
		{
			hlock->ancestor_recursive_mode_ = mode_granted;
		} else {
			hlock->ancestor_recursive_mode_ = lock_protocol::Mode::NL;
		}
		hlock->lock_ = lm_->FindLock(lid);
		hlock->lock_->payload_ = static_cast<void*>(hlock);
		r = lock_protocol::OK;
	}

	return r;
}



/// \brief Acquires and attaches a public lock to a hierarchical lock
/// 
/// Invariant: to acquire a public lock, all the lock's ancestors 
/// must already have a public lock.
///
/// Assumes caller holds hlock's mutex or has the lock set at CONVERTING state
/// Caller is also responsible for setting mode and status of the hierarchical 
/// lock after attachment
///
/// caller_has_parent:  caller has the parent lock either locked or locked-converting
lock_protocol::status
HLockManager::AttachPublicLock(HLock* hlock, lock_protocol::Mode mode, 
                               bool caller_has_parent, int flags)
{
	Lock*                    lock;
	HLock*                   phlock = hlock->parent_; // parent hierarchical lock
	LockId                   lid = hlock->lid_;
	LockId                   plid;
	lock_protocol::Mode::Set mode_set;
	lock_protocol::Mode      mode_granted;
	int                      r;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d] Attaching public lock %s to hierarchical lock %s (%s)\n", 
	        id(), lid.c_str(), lid.c_str(), mode.String().c_str());

	mode_set.Insert(mode);
	mode_set.Insert(mode.LeastRecursiveMode());

	if (phlock && phlock->status() != HLock::NONE) {
		if (!caller_has_parent) {
			assert (phlock->BeginConverting(true) == 0);
		}
		if (phlock->lock_) {
			// parent has public lock: by induction, all ancestors have a public lock
			if (lock_protocol::Mode::AbidesHierarchyRule(mode, phlock->lock_->public_mode_)) {
				plid = phlock->lock_->lid_;
				if ((r = lm_->Acquire(lid, mode_set, flags, 1, (void**) &plid, mode_granted)) != 
				    lock_protocol::OK)
				{
					if (r == lock_protocol::DEADLK) {
						DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_hlckmgr), 
								"[%d] Potential deadlock. Must abort\n", id());
					}
					goto done;
				}
			} else {
				r = lock_protocol::HRERR;
				goto done;
			}
		} else {
			if (!caller_has_parent) {
				phlock->EndConverting(true);
			}
			return AttachPublicLockChainUp(hlock, mode, flags);
		}
	} else {
		r = lock_protocol::NOENT;
		return r;
	}

	if (r == lock_protocol::OK) {
		if (mode_granted == lock_protocol::Mode(lock_protocol::Mode::XR) || 
			mode_granted == lock_protocol::Mode(lock_protocol::Mode::SR)) 
		{
			hlock->ancestor_recursive_mode_ = mode_granted;
		} else {
			hlock->ancestor_recursive_mode_ = lock_protocol::Mode::NL;
		}
		if (phlock) {
			phlock->AddChild(hlock);
		}
		hlock->lock_ = lm_->FindLock(lid);
		hlock->lock_->payload_ = static_cast<void*>(hlock);
		r = lock_protocol::OK;
	}

done:
	if (!caller_has_parent) {
		phlock->EndConverting(true);
	}
	return r;
}


/// \brief Acquires and attaches public locks on a chain of hierarchical locks
///
/// Starting at hlock, it walks up the hierarchy till it finds a public lock
/// and then acquires public locks on each node top-to-bottom. The mode of 
/// each public lock is the mode of the corresponding private lock.
/// 
/// For this operation to be safe, the public lock must be a recursive lock.
///
/// Assumes caller holds:
///   hlock's mutex or has the lock set at CONVERTING state
///
lock_protocol::status
HLockManager::AttachPublicLockChainUp(HLock* hlock, lock_protocol::Mode mode, int flags)
{
	struct LockDsc{
		LockId lid;
	};
	Lock*                          lock;
	HLock*                         phlock = hlock->parent_;
	HLock*                         hl;
	HLock*                         old_hl;
	HLock*                         hl_public;
	LockId                         lid = hlock->lid_;
	uint64_t                       plid_u64;
	lock_protocol::Mode::Set       mode_set;
	lock_protocol::Mode            mode_granted;
	int                            r;
	std::vector<HLock*>            hlv;
	std::vector<HLock*>::iterator  hlv_itr; 
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d] Attaching public lock %s to hierarchical lock %s (%s)\n", 
	        id(), lid.c_str(), lid.c_str(), mode.String().c_str());

	// serialize self to anyone traversing the lock hierarchy such as other 
	// AttachPublicLockChainUp and lock revocations. 
	// serializing under a global lock simplifies deadlock avoidance.
	pthread_mutex_lock(&mutex_);
	while (status_ != NONE) {
		pthread_cond_wait(&status_cv_, &mutex_);
	}
	status_ = ATTACHING;
	pthread_mutex_unlock(&mutex_);

	// mark all locks up to the ancestor that has a public lock attached 
	// as CONVERTING
	hl = hlock->parent_;
	pthread_mutex_lock(&hl->mutex_);
	while (!hl->lock_) { 
		assert(hl->BeginConverting(false) == 0);
		hlv.push_back(hl);
		old_hl = hl;
		hl = hl->parent_;
		pthread_mutex_lock(&hl->mutex_);
		pthread_mutex_unlock(&old_hl->mutex_);
	}
	hl_public = hl; // the hlock having a public lock attached.
	assert(hl->lock_->public_mode_ == lock_protocol::Mode::XR ||
	       hl->lock_->public_mode_ == lock_protocol::Mode::SR);
	pthread_mutex_unlock(&hl->mutex_);
	
	// acquire and attach public locks top-to-bottom
	for (hlv_itr = hlv.end()-1; hlv_itr >= hlv.begin(); hlv_itr--) {
		hl = *hlv_itr;
		lid = hl->lid_;
		assert(hl->parent_ && hl->parent_->lock_);
		plid_u64 = hl->parent_->lock_->lid_.marshall();
		// ensure we get a public lock under a recursive ancestor, 
		// otherwise lock acquisition is not guaranteed. 
		assert(lock_protocol::Mode::AbidesRecursiveRule(hl_public->lock_->public_mode_, 
		                                                hl->mode_));
		r = lm_->Acquire(lid, hl->mode_, 0, 1 /* argc */, (void**) &plid_u64, mode_granted);
		assert(r == lock_protocol::OK);
		pthread_mutex_lock(&hl->mutex_);
		hl->lock_ = lm_->FindLock(lid);
		hl->lock_->payload_ = static_cast<void*>(hl);
		if (hl == phlock) {
			hl->AddChild(hlock);
		}
		hl->EndConverting(false);
		pthread_mutex_unlock(&hl->mutex_);
	}
	// attach public lock to the lock
	assert(hlock->parent_ && hlock->parent_->lock_);
	plid_u64 = hlock->parent_->lock_->lid_.marshall();
	r = lm_->Acquire(hlock->lid_, mode, 0, 1, (void**) &plid_u64, mode_granted); 
	hlock->lock_ = lm_->FindLock(lid);
	hlock->lock_->payload_ = static_cast<void*>(hlock);
	
	pthread_mutex_lock(&mutex_);
	status_ = NONE;
	pthread_cond_broadcast(&status_cv_);
	pthread_mutex_unlock(&mutex_);

	return r;
}


/// \brief Acquires and attaches a public lock on a child
///
/// It propagates a parent lock by acquiring a public lock on a child
/// of the same mode as the parent's. 
/// If a child has already a public lock attached (e.g. by AttachPublicLockChainUp) 
/// then we convert the public lock to the supremum mode.
lock_protocol::status 
HLockManager::AttachPublicLockToChild(HLock* phlock, HLock* chlock, lock_protocol::Mode mode)
{
	lock_protocol::status  r;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d] Attach public lock (%s) to child %s of hierarchical lock %s.\n", 
	        id(), mode.String().c_str(), chlock->lid_.c_str(), phlock->lid_.c_str());

	assert(phlock->status() == HLock::LOCKED_CONVERTING || phlock->status() == HLock::CONVERTING);
	assert(chlock->status() == HLock::LOCKED_CONVERTING || chlock->status() == HLock::CONVERTING) ;

	//if (!lock_protocol::Mode::AbidesHierarchyRule(mode, phlock->lock_->public_mode_)) {
	//	return lock_protocol::HRERR;
	//}

	if (chlock->lock_) {	
		// someone attached a lock from bottom-to-up using the method
		// AttachPublicLockChainUp. just convert the public lock to the 
		// supremum mode. the convert method should not block and should succeed.
		lock_protocol::Mode mode = lock_protocol::Mode::Supremum(mode, chlock->mode_);
		assert(lm_->Convert(chlock->lock_, mode, true) == lock_protocol::OK);
	} else {
		if ((r = AttachPublicLock(chlock, mode, true, HLock::FLG_NOBLK)) == 
			lock_protocol::RETRY) 
		{
			// I have the recursive lock but couldn't grab a public lock on my
			// child. By invariant, this is only possible when a child is 
			// reachable through multiple paths. Thus, I can safely ignore this.
			DBG_LOG(DBG_WARNING, DBG_MODULE(client_hlckmgr), 
					"[%d] Cannot acquire public lock on child. "
					"Child potentially reachable through multiple paths.\n", id());
		}
	}
	return lock_protocol::OK;
}


/// \brief Acquires and attaches public locks on a set of hierarchical locks
///
/// OBSOLETE 
///
/// Assumes:
///  caller has marked hlock as CONVERTING
/// FIXME: this method checks that the child lock abides the hierarchy rule 
/// on the parent public mode. This means we should really check on the new public 
/// mode of the parent or require the caller to set the public mode of the parent
/// to the new mode. Currently we do the latter which makes it a very bad API.
lock_protocol::status 
HLockManager::AttachPublicLockToChildren(HLock* hlock, lock_protocol::Mode mode)
{
	HLockPtrSet::iterator itr;
	lock_protocol::status r;
	HLock*                hl;
	HLockPtrSet&          hlock_set = hlock->children_; 

	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d] Attaching public locks (%s) to children of hierarchical lock %s.\n", 
	        id(), mode.String().c_str(), hlock->lid_.c_str());

	assert(hlock->status() == HLock::LOCKED_CONVERTING || hlock->status() == HLock::CONVERTING);

	if (hlock_set.size() == 0) {
		// no members exist so there is nothing to do
		return lock_protocol::OK;
	}
	if (!lock_protocol::Mode::AbidesHierarchyRule(mode, hlock->lock_->public_mode_)) {
		return lock_protocol::HRERR;
	}

	for (itr = hlock_set.begin(); itr != hlock_set.end(); itr++) {
		hl = *itr;
		assert(hl->BeginConverting(true) == 0);
		assert(AttachPublicLockToChild(hlock, hl, mode)==0);
		hl->EndConverting(true);
	}
	return lock_protocol::OK;
}


lock_protocol::status
HLockManager::AcquireInternal(pthread_t tid, HLock* hlock, HLock* phlock,
                              lock_protocol::Mode mode, int flags)
{
	lock_protocol::status r;
	lock_protocol::Mode   mode_granted;
	LockId                lid = hlock->lid_;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d:%lu] Acquiring hierarchical lock %s (%s)\n", id(), 
			tid, lid.c_str(), mode.String().c_str());
	
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
			            "[%d:%lu] hierarchical lock %s free locally (base lock mode %s): "
					    "grant to thread %lu\n",
			            id(), tid, lid.c_str(), 
			            hlock->lock_->public_mode_.String().c_str(), tid);
				if (mode < hlock->mode_) {
					// silent acquisition covered by private mode
					hlock->owner_ = tid;
					hlock->set_status(HLock::LOCKED);
					r = lock_protocol::OK;
					break;
				} else {
					if (mode < hlock->lock_->public_mode_) {
						// silent acquisition covered by public mode
						hlock->mode_ = lock_protocol::Mode::Supremum(hlock->mode_, mode);
						assert((hlock->mode_ == hlock->lock_->public_mode_) ||
						       (hlock->mode_ < hlock->lock_->public_mode_));
						hlock->owner_ = tid;	
						hlock->set_status(HLock::LOCKED);
						r = lock_protocol::OK;
					} else {
						// must convert public lock
						DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_hlckmgr), 
								"Must convert public lock\n");
					}
				}
			} else {
				//FIXME: instead of checking the ancestor_recursive_mode, we check 
				// whether we are covered by our most recent acnestor. otherwise, we 
				// follow up the parent chain to find the ancestor that covers us. 
				// invariant: the lock has not been dropped so there must be an ancestor
				// that covers us. otherwise our lock should had been revoked.
				// we'll need to grab the mutex of the ancestor to check its state
				// FIXME: What if the old parent (hlock->parent_) is different from
				// the new parent (phlock)?
				if (hlock->parent_) {
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
			        "[%d:%lu] lock %s: another thread in acquisition\n",
			        id(), tid, lid.c_str());
			if (flags & Lock::FLG_NOBLK) {
				r = lock_protocol::RETRY;
				break;
			}
			while (hlock->status() == HLock::ACQUIRING) {
				pthread_cond_wait(&hlock->status_cv_, &hlock->mutex_);
			}
			goto check_state;	// Status changed. Re-check.
		case HLock::LOCKED_CONVERTING:
		case HLock::CONVERTING:
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
			        "[%d:%lu] hierarchical lock manager is converting lock %s\n",
			        id(), tid, lid.c_str());
			while (hlock->status() == HLock::CONVERTING || 
			       hlock->status() == HLock::LOCKED_CONVERTING) 
			{
				pthread_cond_wait(&hlock->status_cv_, &hlock->mutex_);
			}
			goto check_state;	// Status changed. Re-check.
		case HLock::LOCKED:
			if (hlock->owner_ == tid) {
				// current thread has already obtained the lock
				DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
				        "[%d:%lu] current thread already got lock %s\n",
				        id(), tid, lid.c_str());
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
			dbg_log(DBG_INFO, "[%d:%lu] Hierarchical lock %s not available; acquiring now\n",
			        id(), tid, lid.c_str());
			hlock->set_status(HLock::ACQUIRING);
			if (phlock) {
				hlock->parent_ = phlock;
				pthread_mutex_lock(&phlock->mutex_);
				if (flags & HLock::FLG_PUBLIC ||
					!lock_protocol::Mode::AbidesRecursiveRule(mode, phlock->ancestor_recursive_mode_))
				{
					pthread_mutex_unlock(&phlock->mutex_);
					r = AttachPublicLock(hlock, mode, false, flags);
					if (r == lock_protocol::OK) {
						hlock->mode_ = mode;
						hlock->owner_ = tid;
						hlock->set_status(HLock::LOCKED);
					} else {
						hlock->set_status(HLock::NONE);
					}
				} else {	
					phlock->AddChild(hlock);
					hlock->ancestor_recursive_mode_ = phlock->ancestor_recursive_mode_;
					hlock->mode_ = lock_protocol::Mode::Supremum(hlock->mode_, mode);
					hlock->owner_ = tid;
					hlock->set_status(HLock::LOCKED);
					r = lock_protocol::OK;
					pthread_mutex_unlock(&phlock->mutex_);
				}
			} else {
				r = AttachPublicLockCapability(hlock, mode, flags);
				if (r == lock_protocol::OK) {
					hlock->mode_ = mode;
					hlock->owner_ = tid;
					hlock->set_status(HLock::LOCKED);
				} else {
					hlock->set_status(HLock::NONE);
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

	r = AcquireInternal(pthread_self(), hlock, NULL, mode, flags);
	return r;
}


lock_protocol::status
HLockManager::Acquire(HLock* hlock, HLock* phlock, lock_protocol::Mode mode, int flags)
{
	lock_protocol::status r;

	r = AcquireInternal(pthread_self(), hlock, phlock, mode, flags);
	return r;
}


lock_protocol::status
HLockManager::Acquire(LockId lid, LockId plid, 
                      lock_protocol::Mode mode, int flags)
{
	lock_protocol::status r;
	HLock*                hlock;
	HLock*                phlock;

	pthread_mutex_lock(&mutex_);
	hlock = FindOrCreateLockInternal(lid);
	phlock = FindLockInternal(plid);
	pthread_mutex_unlock(&mutex_);
	if (phlock == NULL) {
		return lock_protocol::NOENT;
	}
	r = AcquireInternal(pthread_self(), hlock, phlock, mode, flags);
	return r;
}


lock_protocol::status
HLockManager::Acquire(LockId lid, 
                      lock_protocol::Mode mode, int flags)
{
	lock_protocol::status r;
	HLock*                hlock;

	hlock = FindOrCreateLock(lid);
	r = AcquireInternal(pthread_self(), hlock, NULL, mode, flags);
	return r;
}


/** 
 * \param if force is set then lock is released even if caller thread is not  
 * the owner of the lock
 */
lock_protocol::status
HLockManager::ReleaseInternal(pthread_t tid, HLock* hlock, bool force)
{
	lock_protocol::status r = lock_protocol::OK;
	LockId                lid = hlock->lid_;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d:%lu] Releasing hierarchical lock %s\n", id(), tid, lid.c_str()); 
	
	if (hlock->owner_ == tid) {
		if (hlock->status() == HLock::LOCKED_CONVERTING) {
			hlock->set_status(HLock::CONVERTING);
		} else {
			hlock->set_status(HLock::FREE);
		}
	} else {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d:%lu] thread %lu is not holder of hierarchical lock %s\n",
		        id(), tid, tid, lid.c_str());
		r = lock_protocol::NOENT;
	}

	return r;
}


lock_protocol::status
HLockManager::Release(HLock* hlock)
{
	lock_protocol::status r;

	pthread_mutex_lock(&hlock->mutex_);
	r = ReleaseInternal(pthread_self(), hlock, false);
	pthread_mutex_unlock(&hlock->mutex_);
	return r;
}


lock_protocol::status
HLockManager::Release(LockId lid)
{
	lock_protocol::status r;
	HLock*                hlock;

	pthread_mutex_lock(&mutex_);
	hlock = FindLockInternal(lid);
	pthread_mutex_unlock(&mutex_);
	if (hlock == NULL) {
		DBG_LOG(DBG_WARNING, DBG_MODULE(client_hlckmgr),
		        "Unknown hierarchical lock %s\n", lid.c_str());
		return lock_protocol::NOENT;
	}
	pthread_mutex_lock(&hlock->mutex_);
	r = ReleaseInternal(pthread_self(), hlock, false);
	pthread_mutex_unlock(&hlock->mutex_);
	return r;
}


/** 
 * \brief Recursively revokes a lock and its subtree
 *
 * Note: we could implement the following optimization but we didn't as we 
 * don't expect high benefit: when new public mode does not cover the private
 * mode, we could convert the lock to the new mode and then recursively 
 * downgrade each descendant to a mode that does not violate the hierarchy rule.
 * Insted we just release all descendant locks.
 */
lock_protocol::status
HLockManager::DowngradePublicLockRecursive(HLock* hlock,
                                           lock_protocol::Mode new_public_mode, 
                                           HLockPtrLockModePairSet* release_set)
{
	HLockPtrSet::iterator itr;
	lock_protocol::Mode   old_public_mode;
	
	hlock->BeginConverting(true);
	if (!hlock->lock_) {
		for (itr = hlock->children_.begin(); itr != hlock->children_.end(); itr++) {
			HLock* hl = *itr;
			assert(DowngradePublicLockRecursive(hl, lock_protocol::Mode::NL, release_set) == 0);
		}
		release_set->push_back(HLockPtrLockModePair(hlock, lock_protocol::Mode::NL));
	} else {
		old_public_mode = hlock->lock_->public_mode_;
		// check whether new_public_mode covers lock's private mode
		if (new_public_mode > hlock->mode_ || new_public_mode == hlock->mode_) 
		{
			// new_public_mode covers lock's private mode.
			// check whether the public lock we release is a recursive lock
			// as we need to propagate it down to the children
			if (old_public_mode == lock_protocol::Mode::XR ||
				old_public_mode == lock_protocol::Mode::SR) 
			{
				if (lock_protocol::Mode::AbidesHierarchyRule(old_public_mode, new_public_mode) 
					== lock_protocol::OK) 
				{
					for (itr = hlock->children_.begin(); itr != hlock->children_.end(); itr++) {
						HLock* hl = *itr;
						assert(hl->BeginConverting(true) == 0);
						assert(AttachPublicLockToChild(hlock, hl, old_public_mode) == 0);
						assert(hl->ancestor_recursive_mode_ == old_public_mode);
						hl->EndConverting(true);
					}
				} else {
					// cannot propagate recursive lock down the tree as we would 
					// violate the hierarchy rule: drop children subtrees
					for (itr = hlock->children_.begin(); itr != hlock->children_.end(); itr++) {
						HLock* hl = *itr;
						assert(DowngradePublicLockRecursive(hl, lock_protocol::Mode::NL, release_set) == 0);
					}
				}
			}	
			release_set->push_back(HLockPtrLockModePair(hlock, new_public_mode));
		} else {
			// new mode no longer covers lock's private mode. 
			// we release the lock and every descendant of the lock 
			release_set->push_back(HLockPtrLockModePair(hlock, lock_protocol::Mode::NL));
			for (itr = hlock->children_.begin(); itr != hlock->children_.end(); itr++) {
				HLock* hl = *itr;
				assert(DowngradePublicLockRecursive(hl, lock_protocol::Mode::NL, release_set) == 0);
			}
		}
	}
	return 0;
}


lock_protocol::status
HLockManager::DowngradePublicLock(HLock* hlock, lock_protocol::Mode new_mode)
{
	HLockPtrLockModePairSet::iterator itr;
	HLockPtrLockModePairSet           release_set;

	release_set.clear();
	assert(DowngradePublicLockRecursive(hlock, new_mode, &release_set) == 0);
	
	if (hcb_) {
		hcb_->PreDowngrade();
	}
	
	// downgrade locks
	for (itr = release_set.begin(); itr != release_set.end(); itr++) {
		HLock*              hl = itr->hlock_;
		lock_protocol::Mode new_mode = itr->mode_;
		if (new_mode == lock_protocol::Mode::NL) {
			DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
				    "[%d] Revoking hierarchical lock %s\n", id(), hl->lid_.c_str()); 
			// if lock is LOCKED then wait to be released. 
			// may need to abort any other dependent lock requests to avoid deadlock?
			pthread_mutex_lock(&hl->mutex_);
			hl->WaitStatus(HLock::CONVERTING);
			pthread_mutex_unlock(&hl->mutex_);
			if (hcb_) {
				hcb_->OnRelease(hl);
			}
			if (hl->lock_) {
				assert(lm_->Release(hl->lock_, true) == lock_protocol::OK); 
			}
			// clearing lock
			hl->ancestor_recursive_mode_ = lock_protocol::Mode::NL;
			hl->lock_ = NULL;
			hl->parent_ = NULL;
			hl->set_status(HLock::NONE); // I should have called EndConverting but it
			                             // cannot set status to NONE
		} else {
			DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
				    "[%d] Downgrading hierarchical lock %s: %s to %s\n", id(), hl->lid_.c_str(), 
				    hl->mode_.String().c_str(), new_mode.String().c_str()); 
			// even if lock is LOCKED we don't need to wait for it to be released because
			// we are converting it to a mode compatible with the currently locked mode
			if (hcb_) {
				hcb_->OnConvert(hl);
			}
			if (hl->lock_) {
				assert(lm_->Convert(hl->lock_, new_mode, true) == lock_protocol::OK); 
			}
			hl->ancestor_recursive_mode_ = lock_protocol::Mode::NL;
			hl->EndConverting(true);
		}
	}

	return 0;
}


int
HLockManager::Revoke(Lock* lp, lock_protocol::Mode new_mode)
{
	// ISSUE
	// CHALLENGE: if you come here via a synchronous call by the base lock manager
	// then you can't block when trying to upgrade the children's locks. 
	// you either need to respond to the caller immediately and acquire locks 
	// asynchronously in a different thread (i.e. context switch or user level 
	// thread package) or need to try to acquire locks without blocking. 
	//
	// The lock protocol should guarantee whether you can acquire locks without 
	// blocking. for example, if you already have a recursive lock then you know 
	// that you can acquire a recursive lock on the children as well. so there 
	// are several children upgrades which you know can happen. 
	// There is an exemption to the rule though: we can't guarantee non-blocking 
	// acquisition of a child that is reachable through multiple paths as the 
	// child might already be locked (publicly by another client). We don't need
	// to propagate the recursive lock to that child. The reason is that the 
	// recursive lock on the parent doesn't have any effect on the child as it was 
	// not actually locked by us (if the chold is locked then it must have a public
	// lock).
	
	int    ret;
	HLock* hlock;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr), 
	        "[%d] Revoking hierarchical lock %s: %s to %s\n", id(), lp->lid_.c_str(), 
	        lp->public_mode_.String().c_str(), new_mode.String().c_str()); 
	
	// serialize self to other revocations and to other threads that try to 
	// traverse the lock hierarchy such as AttachPublicLockChainUp
	pthread_mutex_lock(&mutex_);
	while (status_ != NONE) {
		if (status_ == SHUTTING_DOWN) {
			pthread_mutex_unlock(&mutex_);
			return -1;
		}
		pthread_cond_wait(&status_cv_, &mutex_);
	}
	status_ = REVOKING;
	pthread_mutex_unlock(&mutex_);

	hlock = static_cast<HLock*>(lp->payload_);
	ret = DowngradePublicLock(hlock, new_mode);

	pthread_mutex_lock(&mutex_);
	status_ = NONE;
	pthread_cond_broadcast(&status_cv_);
	pthread_mutex_unlock(&mutex_);

	return ret;
}


void
HLockManager::PrintDebugInfo()
{
	LockMap::iterator itr;
	for (itr = locks_.begin(); itr != locks_.end(); ++itr) {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_hlckmgr),
		        "LOCK[%s] = %p\n", itr->first.c_str(), itr->second);
	}
}

} // namespace client
} // namespace cc
} // namespace ssa
