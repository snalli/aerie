// RPC stubs for clients to talk to lock_server, and cache the locks
// see lckmgr.h for protocol details.

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
	  waiting_clients_(0), 
	  can_retry_(false),
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
		if (sts == LOCKED) {
			owner_ = pthread_self();
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
	pthread_t          th;

	pthread_mutex_init(&mutex_, NULL);
	pthread_mutex_init(&revoke_mutex_, NULL);
	pthread_cond_init(&revoke_cv, NULL);

	Locks_.set_empty_key(-1);

	/* register client's lock manager RPC handlers with srv2cl_ */
	srv2cl_->reg(rlock_protocol::revoke, this, &LockManager::revoke);
	srv2cl_->reg(rlock_protocol::retry, this, &LockManager::retry);
	int r = pthread_create(&th, NULL, &releasethread, (void *) this);
	assert (r == 0);
}


LockManager::~LockManager()
{
	pthread_mutex_lock(&mutex_);
	google::dense_hash_map<lock_protocol::LockId, Lock*>::iterator itr;
	for (itr = Locks_.begin(); itr != Locks_.end(); ++itr) {
		if (itr->second->status() == Lock::FREE) {
			do_release(itr->second);
		} else if (itr->second->status() == Lock::LOCKED
		           && pthread_self() == itr->second->owner_) 
		{
			ReleaseInternal(itr->second);
			do_release(itr->second);
		}
		// TODO what about other states?
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
		dbg_log(DBG_INFO, "[%d] releasing lock %llu at seq %d\n", cl2srv_->id(),
		        lid, seq);
		Lock* l = Locks_[lid];
		if (l->status() == Lock::NONE) {
			dbg_log(DBG_WARNING, "[%d] false revoke alarm: %llu\n", cl2srv_->id(), lid);
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
		while (l->status() != Lock::FREE) {
			// wait until the lock is released 
			pthread_cond_wait(&l->status_cv_, &mutex_);
		}
		dbg_log(DBG_INFO, "[%d] calling release RPC for lock %llu\n", cl2srv_->id(),
		        lid);
		if (do_release(l) == lock_protocol::OK) {
			// we set the lock's status to NONE instead of erasing it
			l->set_status(Lock::NONE);
			revoke_map_.erase(lid);
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
/// for this reason, we need an ACQUIRING status to tell other threads that
/// an acquisition is in progress.
inline lock_protocol::status
LockManager::AcquireInternal(Lock* l)
{
	lock_protocol::status r;
	lock_protocol::LockId lid = l->lid_;

	switch (l->status()) {
		case Lock::FREE:
			// great! no one is using the cached lock
			dbg_log(DBG_INFO, "[%d] lock %llu free locally: grant to %lu\n",
			        cl2srv_->id(), lid, (unsigned long)pthread_self());
			r = lock_protocol::OK;
			l->set_status(Lock::LOCKED);
			break;
		case Lock::ACQUIRING:
			// There is on-going lock acquisition; we just sit here and wait
			// its completion, in which case we can safely fall through to the
			// next case
			dbg_log(DBG_INFO, "[%d] lck-%llu: another thread in acquisition\n",
			        cl2srv_->id(), lid);
			while (l->status() == Lock::ACQUIRING) {
				pthread_cond_wait(&l->status_cv_, &mutex_);
			}
			if (l->status() == Lock::FREE) {
				// somehow this lock becomes mine!
				r = lock_protocol::OK;
				l->set_status(Lock::LOCKED);
				break;
			}
		// the lock is LOCKED or NONE, so we continue to the next case
		case Lock::LOCKED:
			if (l->owner_ == pthread_self()) {
				// the current thread has already obtained the lock
				dbg_log(DBG_INFO, "[%d] current thread already got lck %llu\n",
				        cl2srv_->id(), lid);
				r = lock_protocol::OK;
				break;
			} else {
				// in the predicate of the while loop, we don't check if the lock is
				// revoked by the server. this allows competition between the local
				// threads and the revoke thread.
				while (l->status() != Lock::FREE && l->status() != Lock::NONE) {
					// TODO also check if there are many clients waiting
					pthread_cond_wait(&l->status_cv_, &mutex_);
				}
				if (l->status() == Lock::FREE) {
					dbg_log(DBG_INFO, "[%d] lck %llu obatained locally by th %lu\n",
							cl2srv_->id(), lid, (unsigned long)pthread_self());
					r = lock_protocol::OK;
					l->set_status(Lock::LOCKED);
					break;
				}
				// if we reach here, it means the lock has been returned to the
				// server, i.e., l->status() == Lock::NONE. we just fall through
			}
		case Lock::NONE:
			dbg_log(DBG_INFO, "[%d] lock %llu not available; acquiring now\n",
			        cl2srv_->id(), lid);
			l->set_status(Lock::ACQUIRING);
			while ((r = do_acquire(l)) == lock_protocol::RETRY) {
				while (!l->can_retry_) {
					pthread_cond_wait(&l->retry_cv_, &mutex_);
				}
			}
			if (r == lock_protocol::OK) {
				dbg_log(DBG_INFO, "[%d] thread %lu got lock %llu at seq %d\n",
				        cl2srv_->id(), pthread_self(), lid, l->seq_);
				l->set_status(Lock::LOCKED);
			}
			break;
		default:
			break;
	}
	return r;
}


lock_protocol::status
LockManager::Acquire(Lock* lock)
{
	lock_protocol::status r;

	if (last_seq_ == 0) {
		// this is my first contact with the server, so i have to tell him
		// my rpc address to subscribe for async rpc response
		int unused;
		if ((r = cl2srv_->call(lock_protocol::subscribe, cl2srv_->id(), id_, unused)) !=
            lock_protocol::OK) 
		{
			dbg_log(DBG_ERROR, "failed to subscribe client: %u\n", cl2srv_->id());
			return r;
		}
	}

	pthread_mutex_lock(&mutex_);
	r = AcquireInternal(lock);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status
LockManager::Acquire(lock_protocol::LockId lid)
{
	Lock* lock;
	lock_protocol::status r;

	if (last_seq_ == 0) {
		// this is my first contact with the server, so i have to tell him
		// my rpc address to subscribe for async rpc response
		int unused;
		if ((r = cl2srv_->call(lock_protocol::subscribe, cl2srv_->id(), id_, unused)) !=
            lock_protocol::OK) 
		{
			dbg_log(DBG_ERROR, "failed to subscribe client: %u\n", cl2srv_->id());
			return r;
		}
	}

	pthread_mutex_lock(&mutex_);
	lock = GetOrCreateLockInternal(lid);
	r = AcquireInternal(lock);
	pthread_mutex_unlock(&mutex_);
	return r;
}


// release() is an atomic operation
inline lock_protocol::status
LockManager::ReleaseInternal(Lock* l)
{
	lock_protocol::status r = lock_protocol::OK;
	lock_protocol::LockId lid = l->lid_;

	if (l->status() == Lock::LOCKED && l->owner_ == pthread_self()) {
		assert(l->used_);
		if (l->waiting_clients_ >= 5) {
			// too many contending clients - we have to relinquish the lock
			// right now
			dbg_log(DBG_WARNING,
					"[%d] more than 5 clients waiting on lck %llu; release now\n",
					cl2srv_->id(), lid);
			revoke_map_.erase(lid);
			do_release(l);
			// mark this lock as NONE anyway
			l->set_status(Lock::NONE);
		} else {
			l->set_status(Lock::FREE);
		}
	} else { 
		dbg_log(DBG_INFO, "[%d] thread %lu is not owner of lck %llu\n",
		        cl2srv_->id(), (unsigned long)pthread_self(), lid);
		r = lock_protocol::NOENT;
	}
	return r;
}


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
LockManager::revoke(lock_protocol::LockId lid, int seq, int &unused)
{
	rlock_protocol::status r = rlock_protocol::OK;

	dbg_log(DBG_INFO,
	        "[%d] server request to revoke lck %llu at seq %d\n", 
	        cl2srv_->id(), lid, seq);
	// we do nothing but pushing back the lock id to the revoke queue
	pthread_mutex_lock(&revoke_mutex_);
	revoke_map_[lid] = seq;
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
		assert(l->status() == Lock::ACQUIRING);
		dbg_log(DBG_INFO, "[%d] retry message for lid %llu seq %d\n",
		        cl2srv_->id(), lid, seq);
		l->can_retry_ = true;
		pthread_cond_signal(&l->retry_cv_);
	} else {
		dbg_log(DBG_WARNING,
		        "[%d] outdated retry %d, current seq for lid %llu is %d\n", 
		        cl2srv_->id(), seq, lid, l->seq_);
	}

	pthread_mutex_unlock(&mutex_);
	return r;
}


// assumes the current thread holds the mutex_
int
LockManager::do_acquire(Lock* l)
{
	int                   r;
	int                   queue_len;
	lock_protocol::LockId lid = l->lid_;

	dbg_log(DBG_INFO, "[%d] calling acquire rpc for lck %llu id=%d seq=%d\n",
	        cl2srv_->id(), lid, cl2srv_->id(), last_seq_+1);
	r = cl2srv_->call(lock_protocol::acquire, cl2srv_->id(), ++last_seq_, lid, queue_len);
	l->seq_ = last_seq_;
	if (r == lock_protocol::OK) {
		l->waiting_clients_ = queue_len;
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

	dbg_log(DBG_INFO, "[%d] calling release rpc for lck %llu id=%d seq=%d\n",
	        cl2srv_->id(), l->lid_, cl2srv_->id(), l->seq_);
	if (lu) {
		lu->dorelease(l->lid_);
	}
	r = cl2srv_->call(lock_protocol::release, cl2srv_->id(), l->seq_, l->lid_, unused);
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
