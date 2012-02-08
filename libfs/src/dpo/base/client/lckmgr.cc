// RPC stubs for clients to talk to lock_server
// see lckmgr.h for protocol details.
//
// The implementation is based on MIT 6.824 Labs. 

#include "dpo/base/client/lckmgr.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "rpc/rpc.h"
#include "common/debug.h"
#include "dpo/base/common/lock_protocol.h"
#include "dpo/base/common/lock_protocol-static.h"

namespace dpo {
namespace cc {
namespace client {

static int revoke2mode_table[] = {
	/* RVK_NO      */  -1,
	/* RVK_NL      */  lock_protocol::Mode::NL,
	/* RVK_XL2SL   */  lock_protocol::Mode::SL,
	/* RVK_SR2SL   */  lock_protocol::Mode::SL,
	/* RVK_XR2IX   */  lock_protocol::Mode::IX,
	/* RVK_XR2XL   */  lock_protocol::Mode::XL,
	/* RVK_XR2IXSL */  lock_protocol::Mode::IXSL,
	/* RVK_IXSL2IX */  lock_protocol::Mode::IX
};


ThreadRecord::ThreadRecord()
	: tid_(-1), 
	  mode_(lock_protocol::Mode(lock_protocol::Mode::NL))
{

}


ThreadRecord::ThreadRecord(id_t tid, Mode mode)
	: tid_(tid), 
	  mode_(mode)
{

}


Lock::Lock(LockId lid = 0)
	: lid_(lid),
	  seq_(0), 
	  used_(false), 
	  can_retry_(false),
	  cancel_(false),
	  revoke_type_(0),
	  status_(NONE),
	  public_mode_(lock_protocol::Mode(lock_protocol::Mode::NL)),
	  gtque_(lock_protocol::Mode::CARDINALITY, lock_protocol::Mode::NL),
	  payload_(0)
{
	pthread_cond_init(&status_cv_, NULL);
	pthread_cond_init(&used_cv_, NULL);
	pthread_cond_init(&retry_cv_, NULL);
	pthread_cond_init(&got_acq_reply_cv_, NULL);
}


Lock::~Lock()
{
	pthread_cond_destroy(&status_cv_);
	pthread_cond_destroy(&used_cv_);
	pthread_cond_destroy(&retry_cv_);
	pthread_cond_destroy(&got_acq_reply_cv_);
}


// assume the thread holds the mutex mutex_
void Lock::set_status(LockStatus sts)
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
			cancel_ = false;
		}
		status_ = sts;
		pthread_cond_broadcast(&status_cv_);
	}
}


static void* releasethread(void* x)
{
	LockManager* lm = (LockManager*) x;
	lm->Releaser();
	return 0;
}


LockManager::LockManager(rpcc* rpc_client, 
                         rpcs* rpc_server, 
						 std::string id)
	: last_seq_(0),
	  cl2srv_(rpc_client),
	  srv2cl_(rpc_server),
	  id_(id)
{
	int          unused;
	int          r;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] Initialize LockManager\n", cl2srv_->id());

	pthread_mutex_init(&mutex_, NULL);
	pthread_mutex_init(&revoke_mutex_, NULL);
	pthread_cond_init(&revoke_cv, NULL);

	locks_.set_empty_key(LockId(0, 0));
	locks_.set_deleted_key(LockId(-1, 0));
	revoke_map_.set_empty_key(LockId(0, 0));
	revoke_map_.set_deleted_key(LockId(-1, 0));
	releaser_thread_running_ = true;

	for (int i=0; i<LOCK_TYPE_COUNT; i++) {
		lcb_[i] = NULL;
		lrvk_[i] = NULL;
	}
	// register client's lock manager RPC handlers with srv2cl_
	srv2cl_->reg(rlock_protocol::revoke, this, &LockManager::revoke);
	srv2cl_->reg(rlock_protocol::retry, this, &LockManager::retry);
	r = pthread_create(&releasethread_th_, NULL, &releasethread, (void *) this);
	assert (r == 0);

	// contact the server and tell him my rpc address to subscribe 
	// for async rpc response
	if ((r = cl2srv_->call(lock_protocol::subscribe, cl2srv_->id(), id_, unused)) !=
	    lock_protocol::OK) 
	{
		DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr), 
		        "failed to subscribe client: %u\n", cl2srv_->id());
	}

}


LockManager::~LockManager()
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] Shutting down Lock Manager\n", cl2srv_->id());
	
	ShutdownReleaser();


	// release any held locks
	pthread_mutex_lock(&mutex_);
	LockMap::iterator itr;
	for (itr = locks_.begin(); itr != locks_.end(); ++itr) {
		if (itr->second->status() == Lock::FREE) {
			do_release(itr->second, 0);
		} else if (itr->second->status() == Lock::LOCKED) {
			ReleaseInternal(0, itr->second, true);
		}
		// FIXME: what about other states?
	}
	pthread_mutex_unlock(&mutex_);
	pthread_cond_destroy(&revoke_cv);
	pthread_mutex_destroy(&mutex_);
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] Shutting down Lock Manager: DONE\n", cl2srv_->id());
}


void 
LockManager::RegisterLockCallback(LockType type, LockCallback* lcb)
{ 
	assert(type < LOCK_TYPE_COUNT);
	lcb_[type] = lcb; 
}


void 
LockManager::RegisterLockRevoke(LockType type, LockRevoke* lrvk)
{ 
	assert(type < LOCK_TYPE_COUNT);
	lrvk_[type] = lrvk; 
}


void 
LockManager::UnregisterLockCallback(LockType type) 
{
	assert(type < LOCK_TYPE_COUNT);
	lcb_[type] = NULL; 
}


void 
LockManager::UnregisterLockRevoke(LockType type) 
{
	assert(type < LOCK_TYPE_COUNT);
	lrvk_[type] = NULL; 
}


/// \brief Returns the lock lid if it exists, otherwise it returns NULL
/// Assumes caller has the mutex LockManager::mutex_
inline Lock* 
LockManager::FindLockInternal(LockId lid)
{
	Lock* lp;

	// if lid does not exist then the initial value of the slot is 0 (NULL)
	lp = locks_[lid];
	return lp;
}


/// \brief Returns the lock lid. If the lock does not exist, it first creates 
/// the lock.
/// Assumes caller has the mutex LockManager::mutex_
inline Lock* 
LockManager::FindOrCreateLockInternal(LockId lid)
{
	Lock* lp;

	lp = locks_[lid];
	if (lp == NULL) {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] Creating lock %s\n", cl2srv_->id(), lid.c_str());
		lp = new Lock(lid);
		locks_[lid] = lp;
	}	
	return lp;
}


/// \brief Finds the lock identified by lid. 
///
/// \param lid lock identifier. 
/// \return a reference (pointer) to the lock.
///
/// Does no reference counting.
Lock*
LockManager::FindLock(LockId lid)
{
	Lock* l;

	pthread_mutex_lock(&mutex_);
	l = FindLockInternal(lid);
	pthread_mutex_unlock(&mutex_);
	return l;
}


/// \brief Finds the lock identified by lid. If the lock lid does not exist 
/// it first creates the lock.
///
/// \param lid lock identifier. 
/// \return a reference (pointer) to the lock.
///
/// Does no reference counting.
Lock* 
LockManager::FindOrCreateLock(LockId lid)
{
	Lock* l;

	pthread_mutex_lock(&mutex_);
	l = FindOrCreateLockInternal(lid);
	pthread_mutex_unlock(&mutex_);
	return l;
}


/// This method is a continuous loop, waiting to be notified of freed locks
/// that have been revoked by the server, so that it can send a release RPC.
///
/// Some interesting cases: 
/// 1) What if a conversion happens voluntarily by the client before he 
///    receives the revoke request?
///    This is okay as the releaser checks whether the lock is compatible
///    with the requested mode and in such a case he ignores the revoke
///    request. 
void 
LockManager::Releaser()
{
	class LockRevoke* lrvk;

	while (releaser_thread_running_) {
		pthread_mutex_lock(&mutex_);
		while (releaser_thread_running_ && revoke_map_.empty()) {
			pthread_cond_wait(&revoke_cv, &mutex_);
		}
		if (!releaser_thread_running_ && revoke_map_.empty()) {
			pthread_mutex_unlock(&mutex_);
			return;
		}
		RevokeMap::iterator itr = revoke_map_.begin();
		LockId lid = itr->first;
		int   seq = itr->second;
		Lock* l = locks_[lid];
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] releasing/converting lock %s (%s) at seq %d: revoke_type=%d\n", 
		        cl2srv_->id(), lid.c_str(), l->public_mode_.String().c_str(), seq, l->revoke_type_);
		if (l->status() == Lock::NONE) {
			DBG_LOG(DBG_WARNING, DBG_MODULE(client_lckmgr), 
			        "[%d] false revoke alarm: %s\n", cl2srv_->id(), lid.c_str());
			revoke_map_.erase(lid);
			pthread_mutex_unlock(&mutex_);
			continue;
		}
		while (l->seq_ < seq) {
			pthread_cond_wait(&l->got_acq_reply_cv_, &mutex_);
		}
		while (!l->used_) {
			// wait until this lock is used at least once
			pthread_cond_wait(&l->used_cv_, &mutex_);
		}
		// if there is a user manager registered with us then make a synchronous
		// call to revoke the lock. otherwise wait for an asynchronous release 
		assert(lid.type() < LOCK_TYPE_COUNT);
		if (lrvk = lrvk_[lid.type()]) {
			// drop lock to avoid any deadlocks caused by callbacks.
			// releasing the lock is okay as state is consistent here. 
			pthread_mutex_unlock(&mutex_);
			// FIXME RACE: it's possible that some other thread unregisters the  
			// lock user after we make the check above but before we make the call 
			// to Revoke below. in such a case rvk may point to a destroyed object.
			lrvk->Revoke(l, static_cast<lock_protocol::Mode::Enum>(revoke2mode_table[l->revoke_type_]));
		} else {
			lock_protocol::Mode new_mode = lock_protocol::Mode(static_cast<lock_protocol::Mode::Enum>(revoke2mode_table[l->revoke_type_]));
			// wait until the lock is released or becomes compatible with the
			// revoke-request's mode
			while (!((l->status() == Lock::NONE) ||
			         (l->status() == Lock::FREE) || 
					 (l->status() == Lock::LOCKED && 
					  l->revoke_type_ != lock_protocol::RVK_NL &&
					  (l->gtque_.PartialOrder(new_mode) > 0 || l->gtque_.MostSevere() == new_mode)))) 
			{
				pthread_cond_wait(&l->status_cv_, &mutex_);
			}
			// if a synchronous release/convert call has revoked the lock
			// while we were sleeping then we don't need to do anything
			if (revoke_map_.find(lid) != revoke_map_.end()) {	
				if (l->status() == Lock::FREE) {
					DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
							"[%d] calling release RPC for lock %s\n", cl2srv_->id(), lid.c_str());
					if (do_release(l, 0) == lock_protocol::OK) {
						// we set the lock's status to none instead of erasing it
						l->set_status(Lock::NONE);
						revoke_map_.erase(lid);
					}
					// if remote release fails, we leave this lock in the revoke_map_,
					// which will be released in a later attempt
				} else if (l->status() == Lock::LOCKED) {
					DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
							"[%d] calling convert RPC for lock %s\n", cl2srv_->id(), lid.c_str());
					assert(l->gtque_.PartialOrder(new_mode) > 0 || 
					       l->gtque_.MostSevere() == new_mode);
					if (do_convert(l, new_mode, 0) == lock_protocol::OK) {
						l->public_mode_ = new_mode;
						revoke_map_.erase(lid);
					}
					// if remote conversion fails, we leave this lock in the revoke_map_,
					// which will be converted in a later attempt
				} else {
					DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr), 
							"[%d] why woken up?\n", cl2srv_->id());
				}
			}
			pthread_mutex_unlock(&mutex_);
		}
		usleep(500);
	}
}


void 
LockManager::ShutdownReleaser()
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] Shutting down Releaser thread\n", cl2srv_->id());

	// drain the revoke_map so that the releaser thread can exit the 
	// continuous loop and terminate
	pthread_mutex_lock(&mutex_);
	RevokeMap::iterator itri;
	for (itri = revoke_map_.begin(); itri != revoke_map_.end(); itri++) {
		ReleaseInternal(0, locks_[itri->first], false);
	}
	releaser_thread_running_ = false;
	pthread_cond_broadcast(&revoke_cv);
	pthread_mutex_unlock(&mutex_);
	pthread_join(releasethread_th_, NULL);
}


lock_protocol::Mode 
LockManager::SelectMode(Lock* l, lock_protocol::Mode::Set mode_set)
{
	// policy: pick the most severe mode that can be granted locally
	
	lock_protocol::Mode::Set::Iterator itr;
	lock_protocol::Mode                mode;

	for (itr = mode_set.begin(); itr != mode_set.end(); itr++) {
		if ((*itr) == l->public_mode_) {
			mode = *itr;
			break;
		} else {
			if (lock_protocol::Mode::PartialOrder(*itr, mode) > 0 && 
			    lock_protocol::Mode::PartialOrder(*itr, l->public_mode_) < 0 && 
			    l->gtque_.CanGrant(*itr))
			{
				mode = *itr;
			}
		}
	}
	return mode;
}


/// This function blocks until the specified lock is successfully acquired
/// or if an unexpected error occurs.
/// Note that acquire() is NOT an atomic operation because it may temporarily
/// release the mutex_ while waiting on certain condition varaibles.
/// for this reason, we need an ACQUIRING status to tell other threads 
/// that an acquisition is in progress.
lock_protocol::status 
LockManager::AcquireInternal(unsigned long tid, 
                             Lock* l, 
                             lock_protocol::Mode::Set mode_set, 
                             int flags, 
                             int argc, 
                             void** argv,
                             lock_protocol::Mode& mode_granted)
{
	lock_protocol::status r;
	LockId                lid = l->lid_;
	ThreadRecord*         tr;
	lock_protocol::Mode   mode;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] Acquiring lock %s (%s), flags=%d\n", cl2srv_->id(), lid.c_str(), 
			mode_set.String().c_str(), flags);

check_state:
	switch (l->status()) {
		case Lock::FREE:
			// great! no one is using the cached lock
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
			        "[%d] lock %s free locally (public_mode %s): grant to %lu\n",
			        cl2srv_->id(), lid.c_str(), l->public_mode_.String().c_str(), tid);
			mode = SelectMode(l, mode_set);
			if (mode == lock_protocol::Mode(lock_protocol::Mode::NL)) {
				// lock cannot be granted locally. we need to communicate with the server.
				// grant the most severe lock
				mode = mode_set.MostSevere(lock_protocol::Mode::NL);
				if ((r = do_convert(l, mode, flags | Lock::FLG_NOBLK)) 
				    == lock_protocol::OK) 
				{
					l->public_mode_ = mode;
				} else {
					// TODO: ISSUE cannot lock remotely. Modify server to 
					// allow releasing the lock voluntarily so that we 
					// can handle this as the NONE case
					assert(0); 
				}
			}
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
					"[%d:%lu] got lock %s at seq %d\n", 
			        cl2srv_->id(), tid, lid.c_str(), l->seq_);
			mode_granted = mode;
			l->gtque_.Add(ThreadRecord(tid, mode));
			l->set_status(Lock::LOCKED);
			r = lock_protocol::OK;
			break;
		case Lock::ACQUIRING:
			// There is on-going lock acquisition; we just sit here and wait
			// its completion
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
			        "[%d] lock %s: another thread in acquisition\n",
			        cl2srv_->id(), lid.c_str());
			if (flags & Lock::FLG_NOBLK) {
				r = lock_protocol::RETRY;
				break;
			}
			while (l->status() == Lock::ACQUIRING) {
				pthread_cond_wait(&l->status_cv_, &mutex_);
			}
			goto check_state;	// Status changed. Re-check.
		case Lock::LOCKED:
			tr = l->gtque_.Find(tid);
			if (tr) {
				// current thread has already obtained the lock
				DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
				        "[%d:%lu] current thread already got lock %s\n", cl2srv_->id(), tid, lid.c_str());
				mode_granted = tr->mode();
				r = lock_protocol::OK;
			} else {
				// select a mode compatible with the current locked mode 
				lock_protocol::Mode mode = SelectMode(l, mode_set);
				if (mode != lock_protocol::Mode::NL) {
					DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
					        "[%d:%lu] got lock %s at seq %d\n", cl2srv_->id(), tid, lid.c_str(), l->seq_);
					l->gtque_.Add(ThreadRecord(tid, mode));
					mode_granted = mode;
					r = lock_protocol::OK;
				} else {
					if (flags & Lock::FLG_NOBLK) {
						r = lock_protocol::RETRY;
						break;
					}
					pthread_cond_wait(&l->status_cv_, &mutex_);
					goto check_state; // Status changed. Re-check.
				}
			}
			break;
		case Lock::NONE:
			dbg_log(DBG_INFO, "[%d:%lu] lock %s not available; acquiring now\n",
			        cl2srv_->id(), tid, lid.c_str());
			l->set_status(Lock::ACQUIRING);
			while ((r = do_acquire(l, mode_set, flags, argc, argv, mode_granted)) 
			       == lock_protocol::RETRY) 
			{	
				if (flags & Lock::FLG_NOBLK) {
					// if we have to retry but can't block then cancel request
					break;
				}
				while (!(l->can_retry_ || l->cancel_)) {
					pthread_cond_wait(&l->retry_cv_, &mutex_);
				}
				if (l->cancel_) {
					// someone asked us to cancel the request (as it could deadlock)
					dbg_log(DBG_INFO, "[%d:%lu] Cancelling request for lock %s (%s) at seq %d\n",
					        cl2srv_->id(), tid, lid.c_str(), mode_granted.String().c_str(), l->seq_);
					l->set_status(Lock::NONE);
					r = lock_protocol::DEADLK;
					break;
				}
			}
			if (r == lock_protocol::OK) {
				dbg_log(DBG_INFO, "[%d:%lu] got lock %s (%s) at seq %d\n",
				        cl2srv_->id(), tid, lid.c_str(), mode_granted.String().c_str(), l->seq_);
				l->public_mode_ = mode_granted;
				l->gtque_.Add(ThreadRecord(tid, mode_granted));
				l->set_status(Lock::LOCKED);
			} else {
				l->set_status(Lock::NONE);
			} 
			break;
		default:
			break;
	}
	return r;
}


lock_protocol::status 
LockManager::Acquire(Lock* lock, 
                     lock_protocol::Mode::Set mode_set, 
                     int flags, 
                     int argc,
                     void** argv,
                     lock_protocol::Mode& mode_granted)
{
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	r = AcquireInternal(0, lock, mode_set, flags, argc, argv, mode_granted);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status 
LockManager::Acquire(Lock* lock, 
                     lock_protocol::Mode::Set mode_set, 
                     int flags,
                     lock_protocol::Mode& mode_granted)
{
	return Acquire(lock, mode_set, flags, 0, 0, mode_granted);
}


lock_protocol::status
LockManager::Acquire(LockId lid, 
                     lock_protocol::Mode::Set mode_set, 
                     int flags,
					 int argc,
                     void** argv,
                     lock_protocol::Mode& mode_granted)
{
	Lock*                 lock;
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	lock = FindOrCreateLockInternal(lid);
	r = AcquireInternal(0, lock, mode_set, flags, argc, argv, mode_granted);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status 
LockManager::Acquire(LockId lid, 
                     lock_protocol::Mode::Set mode_set, 
                     int flags,
                     lock_protocol::Mode& mode_granted)
{
	return Acquire(lid, mode_set, flags, 0, 0, mode_granted);
}


// this method never blocks and is never queued in the server
lock_protocol::status 
LockManager::ConvertInternal(unsigned long tid, 
                             Lock* l, 
                             lock_protocol::Mode new_mode, 
                             bool synchronous)
{
	lock_protocol::status r = lock_protocol::OK;
	LockId                lid = l->lid_;

	if (l->status() != Lock::LOCKED) {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] cannot convert non-locked lock %s\n",
		        cl2srv_->id(), lid.c_str());
		r = lock_protocol::NOENT;
		return r;
	}

	if (l->gtque_.Exists(tid)) {
		if (new_mode != l->public_mode_ &&
		    lock_protocol::Mode::PartialOrder(new_mode, l->public_mode_) >= 0)  
		{
			// UPGRADE
			// this is an upgrade (new mode after conversion is as restrictive 
			// as or more than the one allowed by global mode) so we need to 
			// communicate with the server. 
			// FIXME: upgrade should consider what happens with l->seq_ and l->used_
			// as it is possible the releaser thread to be in the middle of a revocation.
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
					"[%d] Converting lock %s (upgrade: %s to %s)\n", cl2srv_->id(), lid.c_str(), 
					l->public_mode_.String().c_str(), new_mode.String().c_str());
			if ((r = do_convert(l, new_mode, Lock::FLG_NOBLK)) == lock_protocol::OK) {
				l->public_mode_ = new_mode;
			} else {
				return r;
			}
			// wait for other threads
			while (l->gtque_.ConvertInPlace(tid, new_mode) != 0) {
				while (l->status() == Lock::LOCKED) {
					pthread_cond_wait(&l->status_cv_, &mutex_);
				}
			}
		} else {
			// DOWNGRADE
			// this is a downgrade. we first perform the conversion locally and 
			// then communicate with the server if asked (synchronous==true) or let
			// the releaser thread do it asynchronously for us (synchronous==false)
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
					"[%d] Converting lock %s (downgrade: %s to %s)\n", cl2srv_->id(), lid.c_str(), 
					l->public_mode_.String().c_str(), new_mode.String().c_str());
			assert(l->gtque_.ConvertInPlace(tid, new_mode) == 0);
			if (l->gtque_.Empty()) {
				l->set_status(Lock::FREE);
			} else {
				// signal status_cv_ because there is likely an internal mode change 
				// the releaser thread depends on
				pthread_cond_broadcast(&l->status_cv_);
			}
			if (synchronous) {
				if (do_convert(l, new_mode, 0) == lock_protocol::OK) {
					l->public_mode_ = new_mode;
					revoke_map_.erase(lid);
				}
			}
		}
	} else { 
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] thread %lu is not holder of lock %s\n",
		        cl2srv_->id(), tid, lid.c_str());
		r = lock_protocol::NOENT;
	}
	return r;
}


// release() is an atomic operation
lock_protocol::status
LockManager::Convert(Lock* lock, lock_protocol::Mode new_mode, bool synchronous)
{
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	r = ConvertInternal(0, lock, new_mode, synchronous);
	pthread_mutex_unlock(&mutex_);
	return r;
}


// release() is an atomic operation
lock_protocol::status 
LockManager::Convert(LockId lid, 
                     lock_protocol::Mode new_mode, 
                     bool synchronous)
{
	lock_protocol::status r;
	Lock*                 lock;

	pthread_mutex_lock(&mutex_);
	assert(locks_.find(lid) != locks_.end());
	lock = FindLockInternal(lid);
	r = ConvertInternal(0, lock, new_mode, synchronous);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status 
LockManager::ReleaseInternal(unsigned long tid, 
                             Lock* l, 
                             bool synchronous)
{
	lock_protocol::status r = lock_protocol::OK;
	
	if (!l) {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
				"[%d] Releasing unknown lock\n", cl2srv_->id());
		return lock_protocol::NOENT;
	} else {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
				"[%d] Releasing lock %s \n", cl2srv_->id(), l->lid_.c_str()); 
	}

	if (l->gtque_.Exists(tid)) {
		l->gtque_.Remove(tid);
		if (l->gtque_.Empty()) {
			l->set_status(Lock::FREE);
		}
	} else { 
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] thread %lu is not holder of lock %s\n",
		        cl2srv_->id(), tid, l->lid_.c_str());
		r = lock_protocol::NOENT;
	}

	if (synchronous && (l->status() == Lock::FREE)) {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
				"[%d] calling release RPC for lock %s\n", cl2srv_->id(), l->lid_.c_str());
		if (do_release(l, 0) == lock_protocol::OK) {
			// we set the lock's status to none instead of erasing it
			l->set_status(Lock::NONE);
			revoke_map_.erase(l->lid_);
		}
	}

	return r;
}


// release() is an atomic operation
lock_protocol::status 
LockManager::Release(Lock* lock, bool synchronous)
{
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	r = ReleaseInternal(0, lock, synchronous);
	pthread_mutex_unlock(&mutex_);
	return r;
}


// release() is an atomic operation
lock_protocol::status 
LockManager::Release(LockId lid, bool synchronous)
{
	lock_protocol::status r;
	Lock*                 lock;

	pthread_mutex_lock(&mutex_);
	lock = FindLockInternal(lid);
	r = ReleaseInternal(0, lock, synchronous);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status 
LockManager::CancelLockRequestInternal(Lock* l)
{
	dbg_log(DBG_INFO, "[%d] Cancel lock %s\n", cl2srv_->id(), l->lid_.c_str());
	
	if (l->status() == Lock::ACQUIRING) {
		l->cancel_ = true;
		pthread_cond_signal(&l->retry_cv_);
	}
}


lock_protocol::status 
LockManager::Cancel(Lock* l)
{
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	r = CancelLockRequestInternal(l);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status 
LockManager::Cancel(LockId lid)
{
	Lock*                 lock;
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	lock = FindOrCreateLockInternal(lid);
	r = CancelLockRequestInternal(lock);
	pthread_mutex_unlock(&mutex_);
	return r;
}


rlock_protocol::status 
LockManager::revoke(lock_protocol::LockId lid_u64, int seq, int revoke_type, int &unused)
{
	rlock_protocol::status r = rlock_protocol::OK;
	Lock*                  l;
	LockId                 lid = LockId(lid_u64);

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
	        "[%d] server request to revoke lock %s at seq %d\n", 
	        cl2srv_->id(), lid.c_str(), seq);

	// we do nothing but pushing back the lock id to the revoke queue
	// and marking down the type of revocation
	// ISSUE: we would like to use revoke_mutex_ to protect the revoke
	// queue so that we don't block the server behind the coarse grain 
	// lock mutex_
	pthread_mutex_lock(&mutex_);
	revoke_map_[lid] = seq;
	assert(locks_.find(lid) != locks_.end());
	l = locks_[lid];
	l->revoke_type_ = revoke_type;		
	pthread_cond_signal(&revoke_cv);
	pthread_mutex_unlock(&mutex_);
	return r;
}



/**
 * Tell this client to retry requesting the lock in which this client
 * was interested.
 */
rlock_protocol::status 
LockManager::retry(lock_protocol::LockId lid_u64, 
                   int seq,
                   int& accepted)
{
	rlock_protocol::status r = rlock_protocol::OK;
	Lock*                  l;
	LockId                 lid = LockId(lid_u64);

	pthread_mutex_lock(&mutex_);
	assert(locks_.find(lid) != locks_.end());
	l = FindLockInternal(lid);

	if (seq >= l->seq_) {
		// it doesn't matter whether this retry message arrives before or
		// after the response to the corresponding acquire arrives, as long
		// as the sequence number of the retry matches that of the acquire
		if (l->status() == Lock::NONE) {
			// lock request was cancelled. ignore retry.
			DBG_LOG(DBG_WARNING, DBG_MODULE(client_lckmgr),
		        "[%d] ignoring retry %d for cancelled request, current seq for lid %s is %d\n", 
		        cl2srv_->id(), seq, lid.c_str(), l->seq_);
			accepted = false;
		} else {
			assert(l->status() == Lock::ACQUIRING);
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
					"[%d] retry message for lid %s seq %d\n", cl2srv_->id(), 
					lid.c_str(), seq);
			l->can_retry_ = true;
			accepted = true;
			pthread_cond_signal(&l->retry_cv_);
		}
	} else {
		DBG_LOG(DBG_WARNING, DBG_MODULE(client_lckmgr),
		        "[%d] outdated retry %d, current seq for lid %s is %d\n", 
		        cl2srv_->id(), seq, lid.c_str(), l->seq_);
	}

	pthread_mutex_unlock(&mutex_);
	return r;
}


// assumes the current thread holds the mutex_
int 
LockManager::do_acquire(Lock* l, 
                        lock_protocol::Mode::Set mode_set, 
                        int flags, 
                        int argc,
                        void** argv,
                        lock_protocol::Mode& mode_granted)
{
	int                        r;
	int                        retval;
	int                        rpc_flags=0;
	unsigned long long         arg = 0;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] calling acquire rpc for lock %s id=%d seq=%d\n",
	        cl2srv_->id(), l->lid_.c_str(), cl2srv_->id(), last_seq_+1);

	assert(argc <= 1);
	if (argc > 0) {
		arg = (unsigned long long) argv[0];
	}
	rpc_flags = flags & (lock_protocol::FLG_NOQUE | lock_protocol::FLG_CAPABILITY);
	r = cl2srv_->call(lock_protocol::acquire, cl2srv_->id(), ++last_seq_, 
	                  l->lid_.marshall(), mode_set.value(), rpc_flags, arg, retval);
	l->seq_ = last_seq_;
	if (r == lock_protocol::OK) {
		// great! we have the lock
		mode_granted = static_cast<lock_protocol::Mode::Enum>(retval);
	} else { 
		if (r == lock_protocol::RETRY) {
			l->can_retry_ = false;
		}
		mode_granted = lock_protocol::Mode::NL;
	}
	pthread_cond_signal(&l->got_acq_reply_cv_);
	return r;
}



int 
LockManager::do_convert(Lock* l, lock_protocol::Mode mode, int flags)
{
	int           r;
	int           unused;
	int           rpc_flags;
	LockCallback* lcb;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] calling convert RPC for lock %s id=%d seq=%d\n",
	        cl2srv_->id(), l->lid_.c_str(), cl2srv_->id(), l->seq_);
	assert(l->lid_.type() < LOCK_TYPE_COUNT);
	if (lcb = lcb_[l->lid_.type()]) {
		lcb->OnConvert(l);
	}
	rpc_flags = flags & (lock_protocol::FLG_NOQUE | lock_protocol::FLG_CAPABILITY);
	r = cl2srv_->call(lock_protocol::convert, cl2srv_->id(), l->seq_, l->lid_.marshall(), 
	                  mode.value(), rpc_flags, unused);
	return r;
}


int 
LockManager::do_release(Lock* l, int flags)
{
	int           r;
	int           unused;
	LockCallback* lcb;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] calling release RPC for lock %s id=%d seq=%d\n",
	        cl2srv_->id(), l->lid_.c_str(), cl2srv_->id(), l->seq_);
	assert(l->lid_.type() < LOCK_TYPE_COUNT);
	if (lcb = lcb_[l->lid_.type()]) {
		lcb->OnRelease(l);
	}

	r = cl2srv_->call(lock_protocol::release, cl2srv_->id(), l->seq_, l->lid_.marshall(), 
	                  flags, unused);
	return r;
}


int 
LockManager::stat(LockId lid)
{
	int r;
	int ret = cl2srv_->call(lock_protocol::stat, cl2srv_->id(), lid.marshall(), r);
	
	assert (ret == lock_protocol::OK);
	return r;
}


} // namespace client
} // namespace cc
} // namespace dpo
