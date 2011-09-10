// RPC stubs for clients to talk to lock_server, and cache the locks
// see lckmgr.h for protocol details.
//
// The implementation is based on Yin Qiu's lock server/client,
// which is in turn based on MIT 6.824 Labs. The lock server/client
// has been extended to support exclusive/shared locking and 
// integrate well with the LibFS architecture.

#include "client/lckmgr.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "rpc/rpc.h"
#include "common/debug.h"

namespace client {

Lock::Lock(lock_protocol::LockId lid = 0)
	: lid_(lid),
	  owner_(0), 
	  seq_(0), 
	  used_(false), 
	  can_retry_(false),
	  revoke_type_(0),
	  status_(NONE)
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
void
Lock::set_status(LockStatus sts)
{
	if (status_ != sts) {
		if (sts == LOCKED_X) {
			owner_ = pthread_self();
			used_ = true;
			pthread_cond_signal(&used_cv_);
		}
		if (sts == LOCKED_S) {
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


Lock::LockStatus
Lock::status() const
{
	return status_;
}


static void *
releasethread(void *x)
{
	LockManager* cc = (LockManager*) x;
	cc->releaser();
	return 0;
}


LockManager::LockManager(rpcc* rpc_client, 
                         rpcs* rpc_server, 
						 std::string id,
                         class lock_release_user* _lu)
	: lu(_lu), 
	  last_seq_(0),
	  cl2srv_(rpc_client),
	  srv2cl_(rpc_server),
	  id_(id)
{
	pthread_t    th;
	int          unused;
	int          r;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] Initialize LockManager\n", cl2srv_->id());

	pthread_mutex_init(&mutex_, NULL);
	pthread_mutex_init(&revoke_mutex_, NULL);
	pthread_cond_init(&revoke_cv, NULL);

	Locks_.set_empty_key(-1);

	// register client's lock manager RPC handlers with srv2cl_
	srv2cl_->reg(rlock_protocol::revoke, this, &LockManager::revoke);
	srv2cl_->reg(rlock_protocol::retry, this, &LockManager::retry);
	r = pthread_create(&th, NULL, &releasethread, (void *) this);
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
	        "[%d] Shut down LockManager\n", cl2srv_->id());

	pthread_mutex_lock(&mutex_);
	google::dense_hash_map<lock_protocol::LockId, Lock*>::iterator itr;
	for (itr = Locks_.begin(); itr != Locks_.end(); ++itr) {
		if (itr->second->status() == Lock::FREE_X) {
			do_release(itr->second);
		} else if (itr->second->status() == Lock::LOCKED_X
		           && pthread_self() == itr->second->owner_) 
		{
			ReleaseInternal(itr->second);
			do_release(itr->second);
		}
		// FIXME: what about other states?
	}
	pthread_mutex_unlock(&mutex_);
	pthread_cond_destroy(&revoke_cv);
	pthread_mutex_destroy(&mutex_);
}

// assumes caller has the mutex mutex_
inline Lock*
LockManager::GetOrCreateLockInternal(lock_protocol::LockId lid)
{
	Lock* lockp;

	lockp = Locks_[lid];
	if (lockp == NULL) {
		lockp = new Lock(lid);
		Locks_[lid] = lockp;
	}	
	return lockp;
}


/// Returns a reference (pointer) to the lock
/// Does no reference counting.
Lock*
LockManager::GetOrCreateLock(lock_protocol::LockId lid)
{
	Lock* lockp;

	pthread_mutex_lock(&mutex_);
	lockp = GetOrCreateLockInternal(lid);
	pthread_mutex_unlock(&mutex_);
	return lockp;
}


/// This method is a continuous loop, waiting to be notified of freed locks
/// that have been revoked by the server, so that it can send a release RPC.
void
LockManager::releaser()
{
	running_ = true;
	while (running_) {
		pthread_mutex_lock(&mutex_);
		while (revoke_map_.empty()) {
			pthread_cond_wait(&revoke_cv, &mutex_);
		}
		std::map<lock_protocol::lock_protocol::LockId, int>::iterator itr = revoke_map_.begin();
		lock_protocol::LockId lid = itr->first;
		int seq = itr->second;
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] releasing lock %llu at seq %d\n", cl2srv_->id(), lid, seq);
		Lock* l = Locks_[lid];
		if (l->status() == Lock::NONE) {
			DBG_LOG(DBG_WARNING, DBG_MODULE(client_lckmgr), 
			        "[%d] false revoke alarm: %llu\n", cl2srv_->id(), lid);
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
		while (l->status() != Lock::FREE_S && l->status() != Lock::FREE_X) {
			if (l->status() == Lock::LOCKED_XS && 
			    l->revoke_type_ == 0 /* FIXME */) 
			{
				break;
			}
			// wait until the lock is released 
			// TODO: ping the holder to release/downgrade the lock before I sleep-wait
			// File ISSUE
			pthread_cond_wait(&l->status_cv_, &mutex_);
		}
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] calling release RPC for lock %llu\n", cl2srv_->id(), lid);

		if (l->revoke_type_ == 000 /* FIXME */) {
			if (do_release(l) == lock_protocol::OK) {
				// we set the lock's status to none instead of erasing it
				l->set_status(Lock::NONE);
				revoke_map_.erase(lid);
			}
		} else if (l->revoke_type_ == 000 /* FIXME */) {
			if (do_downgrade(l) == lock_protocol::OK) {
				l->set_status(Lock::FREE_S);
				revoke_map_.erase(lid);
			}
		} else {
			dbg_log(DBG_ERROR, "[%d] unknown revocation type request\n", cl2srv_->id())
		}
		// if remote release fails, we leave this lock in the revoke_map_, which
		// will be released in a later attempt
		pthread_mutex_unlock(&mutex_);
		usleep(500);
	}
}


/// This function blocks until the specified lock is successfully acquired
/// or if an unexpected error occurs.
/// Note that acquire() is NOT an atomic operation because it may temporarily
/// release the mutex_ while waiting on certain condition varaibles.
/// for this reason, we need an ACQUIRING status to tell other threads 
/// that an acquisition is in progress.
inline lock_protocol::status
LockManager::AcquireInternal(Lock* l, bool xmode)
{
	lock_protocol::status r;
	lock_protocol::LockId lid = l->lid_;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] Acquiring lock %llu (%s)\n", cl2srv_->id(), lid, 
	        xmode ? "EXCLUSIVE": "SHARED");

check_state:
	switch (l->status()) {
		case Lock::FREE_X:
			// great! no one is using the cached lock
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
			        "[%d] lock %llu free (X) locally: grant to %lu\n",
			        cl2srv_->id(), lid, (unsigned long)pthread_self());
			r = lock_protocol::OK;
			if (xmode) {
				l->set_status(Lock::LOCKED_X);
			} else {
				l->set_status(Lock::LOCKED_XS);
				l->sharers_.insert(pthread_self());
			}
			break;
		case Lock::FREE_S:
			if (xmode) {
				// First, drop the lock, and then request it at EXCLUSIVE mode
				// FIXME: This step could be done more efficiently but we keep 
				// it simple for now. The best way is to have a special UPGRADE
				// call. 
				// File ISSUE
				if (do_release(l) == lock_protocol::OK) {
					// we set the lock's status to none instead of erasing it
					l->set_status(Lock::NONE);
					// Remove the lock from the revoke set just in case we have 
					// been asked to revoke the lock and we do it here before
					// the releaser thread does it.
					revoke_map_.erase(lid);
					goto check_state;
				}
				DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr),
				        "[%d] Unknown failure for lock %llu\n", 
				        cl2srv_->id(), lid);
			} else {
				dbg_log(DBG_INFO, "[%d] lock %llu free (S) locally: grant to %lu\n",
						cl2srv_->id(), lid, (unsigned long)pthread_self());
				r = lock_protocol::OK;
				l->set_status(Lock::LOCKED_S);
				l->sharers_.insert(pthread_self());
			}	
			break;
		case Lock::ACQUIRING_X:
			// There is on-going lock acquisition; we just sit here and wait
			// its completion
			dbg_log(DBG_INFO, "[%d] lck-%llu: another thread in acquisition\n",
			        cl2srv_->id(), lid);
			while (l->status() == Lock::ACQUIRING_X) {
				pthread_cond_wait(&l->status_cv_, &mutex_);
			}
			// Status changed. Re-check.
			// By the time we wake up, it's possible we find the lock 
			// in the Lock::FREE_[X|S] state. This should be okay as we
			// re-check for any state.
			goto check_state;
		case Lock::LOCKED_S:
			// if requested lock mode (xmode) is SHARED then this is compatible
			// with LOCKED_S so just grab the lock
			if (!xmode) {
				l->sharers_.insert(pthread_self());
				r = lock_protocol::OK;
				break;
			} else {
				// TODO: what? need acquire the lock in X mode, thus fall through to none
				// only if I am the single reader (that is ask for upgrade)
				// Make UPGRADE call
				// To upgrade the lock there must be no other local holders
				// File ISSUE
				assert(0 && "UPGRADE");
			}
		case Lock::LOCKED_XS:
			if (!xmode) {
				l->sharers_.insert(pthread_self());
				r = lock_protocol::OK;
				break;
			} else {
				if (l->sharers_.size() == 1 && 
				    l->sharers_.find(pthread_self()) != l->sharers_.end())
				{
					// local upgrade
					l->sharers_.clear();
					l->set_status(Lock::LOCKED_X);
					r = lock_protocol::OK;
					break;
				}
				pthread_cond_wait(&l->status_cv_, &mutex_);
				goto check_state;
			}
		case Lock::LOCKED_X:
			if (l->owner_ == pthread_self()) {
				// the current thread has already obtained the lock
				// FIXME: need to track the recursive locking count or not allow locking
				dbg_log(DBG_INFO, "[%d] current thread already got lck %llu\n",
				        cl2srv_->id(), lid);
				r = lock_protocol::OK;
				break;
			} else {
				// in the predicate of the while loop, we don't check if the lock is
				// revoked by the server. this allows competition between the local
				// threads and the revoke thread.
				//while (l->status() != Lock::FREE_X && l->status() != Lock::NONE) {
					pthread_cond_wait(&l->status_cv_, &mutex_);
				//}
				// Status changed. Re-check.
				// The lock may be free or returned to the server 
				goto check_state;
			}
		case Lock::NONE:
			dbg_log(DBG_INFO, "[%d] lock %llu not available; acquiring now\n",
			        cl2srv_->id(), lid);
			l->set_status(Lock::ACQUIRING_X);
			while ((r = do_acquire(l, xmode)) == lock_protocol::RETRY) {
				while (!l->can_retry_) {
					pthread_cond_wait(&l->retry_cv_, &mutex_);
				}
			}
			if (r == lock_protocol::OK) {
				dbg_log(DBG_INFO, "[%d] thread %lu got lock %llu at seq %d\n",
				        cl2srv_->id(), pthread_self(), lid, l->seq_);
				if (xmode) {		
					l->set_status(Lock::LOCKED_X);
				} else {
					l->set_status(Lock::LOCKED_S);
					l->sharers_.insert(pthread_self());
				}
			}
			break;
		default:
			break;
	}
	return r;
}


lock_protocol::status
LockManager::AcquireExclusive(Lock* lock)
{
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	r = AcquireInternal(lock, true);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status
LockManager::AcquireExclusive(lock_protocol::LockId lid)
{
	Lock*                 lock;
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	lock = GetOrCreateLockInternal(lid);
	r = AcquireInternal(lock, true);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status
LockManager::AcquireShared(Lock* lock)
{
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	r = AcquireInternal(lock, false);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status
LockManager::AcquireShared(lock_protocol::LockId lid)
{
	Lock*                 lock;
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	lock = GetOrCreateLockInternal(lid);
	r = AcquireInternal(lock, false);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status
LockManager::Acquire(Lock* lock)
{
	return AcquireExclusive(lock);
}


lock_protocol::status
LockManager::Acquire(lock_protocol::LockId lid)
{
	return AcquireExclusive(lid);
}


inline lock_protocol::status
LockManager::ReleaseInternal(Lock* l)
{
	lock_protocol::status r = lock_protocol::OK;
	lock_protocol::LockId lid = l->lid_;

	if (l->status() == Lock::LOCKED_X && l->owner_ == pthread_self()) {
		assert(l->used_);
		l->set_status(Lock::FREE_X);
	} else if (l->status() == Lock::LOCKED_XS) {
		assert(l->used_);
		assert(l->sharers_.empty() == false);
		assert(l->sharers_.erase(pthread_self())==1); // fails if thread didn't lock it
		if (l->sharers_.empty()) {
			l->set_status(Lock::FREE_X);
		}
	} else if (l->status() == Lock::LOCKED_S) {
		assert(l->used_);
		assert(l->sharers_.erase(pthread_self())==1); // fails if thread didn't lock it
		if (l->sharers_.empty()) {
			l->set_status(Lock::FREE_S);
		}
	} else { 
		dbg_log(DBG_INFO, "[%d] thread %lu is not owner of lck %llu\n",
		        cl2srv_->id(), (unsigned long)pthread_self(), lid);
		r = lock_protocol::NOENT;
	}
	return r;
}


// release() is an atomic operation
lock_protocol::status
LockManager::Release(Lock* lock)
{
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	r = ReleaseInternal(lock);
	pthread_mutex_unlock(&mutex_);
	return r;
}


// release() is an atomic operation
lock_protocol::status
LockManager::Release(lock_protocol::LockId lid)
{
	lock_protocol::status r;
	Lock*                 lock;

	pthread_mutex_lock(&mutex_);
	assert(Locks_.find(lid) != Locks_.end());
	lock = GetOrCreateLockInternal(lid);
	r = ReleaseInternal(lock);
	pthread_mutex_unlock(&mutex_);
	return r;
}


rlock_protocol::status
LockManager::revoke(lock_protocol::LockId lid, int seq, int revoke_type, int &unused)
{
	rlock_protocol::status r = rlock_protocol::OK;
	Lock*                  lock;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
	        "[%d] server request to revoke lck %llu at seq %d\n", 
	        cl2srv_->id(), lid, seq);

	// we do nothing but pushing back the lock id to the revoke queue
	// and marking down the type of revocation
	pthread_mutex_lock(&revoke_mutex_);
	revoke_map_[lid] = seq;
	assert(Locks_.find(lid) != Locks_.end());
	lock = Locks_[lid];
	lock->revoke_type_ = revoke_type;		
	pthread_cond_signal(&revoke_cv);
	pthread_mutex_unlock(&revoke_mutex_);
	return r;
}


rlock_protocol::status
LockManager::retry(lock_protocol::LockId lid, int seq,
                   int& current_seq)
{
	rlock_protocol::status r = rlock_protocol::OK;
	Lock*                  l;

	pthread_mutex_lock(&mutex_);
	assert(Locks_.find(lid) != Locks_.end());
	l = GetOrCreateLockInternal(lid);

	if (seq >= l->seq_) {
		// it doesn't matter whether this retry message arrives before or
		// after the response to the corresponding acquire arrives, as long
		// as the sequence number of the retry matches that of the acquire
		assert(l->status() == Lock::ACQUIRING_X);
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] retry message for lid %llu seq %d\n", cl2srv_->id(), 
		        lid, seq);
		l->can_retry_ = true;
		pthread_cond_signal(&l->retry_cv_);
	} else {
		DBG_LOG(DBG_WARNING, DBG_MODULE(client_lckmgr),
		        "[%d] outdated retry %d, current seq for lid %llu is %d\n", 
		        cl2srv_->id(), seq, lid, l->seq_);
	}

	pthread_mutex_unlock(&mutex_);
	return r;
}


// assumes the current thread holds the mutex_
int
LockManager::do_acquire(Lock* l, bool xmode)
{
	lock_protocol::rpc_numbers rpc_number;
	int                        r;
	int                        queue_len;
	lock_protocol::LockId      lid = l->lid_;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] calling acquire rpc for lck %llu id=%d seq=%d\n",
	        cl2srv_->id(), lid, cl2srv_->id(), last_seq_+1);
	//FIXME
	//rpc_number = (xmode) ? lock_protocol::acquire_exclusive
	//                     : lock_protocol::acquire_shared;
	r = cl2srv_->call(rpc_number, cl2srv_->id(), ++last_seq_, lid, queue_len);
	l->seq_ = last_seq_;
	if (r == lock_protocol::OK) {
		// great! we have the lock
	} else if (r == lock_protocol::RETRY) {
		l->can_retry_ = false;
	}
	pthread_cond_signal(&l->got_acq_reply_cv_);
	return r;
}


int
LockManager::do_release(Lock* l)
{
	int r;
	int unused;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] calling release rpc for lck %llu id=%d seq=%d\n",
	        cl2srv_->id(), l->lid_, cl2srv_->id(), l->seq_);
	if (lu) {
		lu->dorelease(l->lid_);
	}
	r = cl2srv_->call(lock_protocol::release, cl2srv_->id(), l->seq_, l->lid_, unused);
	return r;
}


int
LockManager::do_downgrade(Lock* l)
{
	int r;
	int unused;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] calling downgrade rpc for lck %llu id=%d seq=%d\n",
	        cl2srv_->id(), l->lid_, cl2srv_->id(), l->seq_);
	if (lu) {
		lu->dodowngrade(l->lid_);
	}
	r = cl2srv_->call(lock_protocol::downgrade, cl2srv_->id(), l->seq_, l->lid_, unused);
	return r;
}


int
LockManager::stat(lock_protocol::LockId lid)
{
	int r;
	int ret = cl2srv_->call(lock_protocol::stat, cl2srv_->id(), lid, r);
	
	assert (ret == lock_protocol::OK);
	return r;
}


} // namespace client
