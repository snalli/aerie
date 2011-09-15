// RPC stubs for clients to talk to lock_server
// see lckmgr.h for protocol details.
//
// The implementation is based on MIT 6.824 Labs. 

#include "client/lckmgr.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "rpc/rpc.h"
#include "common/debug.h"
#include "common/lock_protocol.h"
#include "common/lock_protocol-static.h"

namespace client {

static int revoke2mode_table[] = {
	/* RVK_NO      */  -1,
	/* RVK_NL      */  lock_protocol::NL,
	/* RVK_XL2SL   */  lock_protocol::SL,
	/* RVK_SR2SL   */  lock_protocol::SL,
	/* RVK_XR2XL   */  lock_protocol::XL,
	/* RVK_IXSL2IX */  lock_protocol::IX
};


ThreadRecord::ThreadRecord()
	: tid_(-1), 
	  mode_(lock_protocol::NL)
{

}


ThreadRecord::ThreadRecord(id_t tid, mode_t mode)
	: tid_(tid), 
	  mode_(mode)
{

}


Lock::Lock(lock_protocol::LockId lid = 0)
	: lid_(lid),
	  seq_(0), 
	  used_(false), 
	  can_retry_(false),
	  revoke_type_(0),
	  status_(NONE),
	  global_mode_(lock_protocol::NL),
	  gtque_(lock_protocol::IXSL+1),
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
void
Lock::set_status(LockStatus sts)
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


static void*
releasethread(void* x)
{
	LockManager* lm = (LockManager*) x;
	lm->Releaser();
	return 0;
}


LockManager::LockManager(rpcc* rpc_client, 
                         rpcs* rpc_server, 
						 std::string id,
						 class LockUser* lu)
	: lu_(lu), 
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

	locks_.set_empty_key(-1);

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
	for (itr = locks_.begin(); itr != locks_.end(); ++itr) {
		if (itr->second->status() == Lock::FREE) {
			do_release(itr->second);
		} else if (itr->second->status() == Lock::LOCKED)
		{
			ReleaseInternal(0, itr->second);
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
	Lock* lp;

	lp = locks_[lid];
	if (lp == NULL) {
		lp = new Lock(lid);
		locks_[lid] = lp;
	}	
	return lp;
}


/// Returns a reference (pointer) to the lock
/// Does no reference counting.
Lock*
LockManager::GetOrCreateLock(lock_protocol::LockId lid)
{
	Lock* l;

	pthread_mutex_lock(&mutex_);
	l = GetOrCreateLockInternal(lid);
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
		Lock* l = locks_[lid];
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

		// wait until the lock is released or becomes compatible with the
		// revoke-request's mode
		while (!((l->status() == Lock::FREE) || 
			     (l->status() == Lock::LOCKED && 
				  l->revoke_type_ != lock_protocol::RVK_NL &&
		          l->gtque_.CanGrant(revoke2mode_table[l->revoke_type_]) ))) 
		{
			// ping the holder to release/downgrade the lock before I sleep-wait
			if (lu_) {
				if (lu_->Revoke(l) >= 0) {
					continue;
				}
			}
			pthread_cond_wait(&l->status_cv_, &mutex_);
		}
		if (l->status() == Lock::FREE) {
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
			        "[%d] calling release RPC for lock %llu\n", cl2srv_->id(), lid);
			if (do_release(l) == lock_protocol::OK) {
				// we set the lock's status to none instead of erasing it
				l->set_status(Lock::NONE);
				revoke_map_.erase(lid);
			}
			// if remote release fails, we leave this lock in the revoke_map_,
			// which will be released in a later attempt
		} else if (l->status() == Lock::LOCKED) {
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
			        "[%d] calling convert RPC for lock %llu\n", cl2srv_->id(), lid);
			assert(l->gtque_.CanGrant(revoke2mode_table[l->revoke_type_]));
			int new_mode = revoke2mode_table[l->revoke_type_];
			if (do_convert(l, new_mode) == lock_protocol::OK) {
				l->global_mode_ = (lock_protocol::mode) new_mode;
				revoke_map_.erase(lid);
			}
			// if remote conversion fails, we leave this lock in the revoke_map_,
			// which will be released in a later attempt
		} else {
			DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_lckmgr), 
			        "[%d] why woken up?\n", cl2srv_->id());
		}
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
LockManager::AcquireInternal(unsigned long tid, Lock* l, int mode, int flags)
{
	lock_protocol::status r;
	lock_protocol::LockId lid = l->lid_;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] Acquiring lock %llu (%s)\n", cl2srv_->id(), lid, 
			lock_protocol::Mode::mode2str(mode).c_str());

check_state:
	switch (l->status()) {
		case Lock::FREE:
			// great! no one is using the cached lock
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
			        "[%d] lock %llu free locally (global_mode %s): grant to %lu\n",
			        cl2srv_->id(), lid, 
			        lock_protocol::Mode::mode2str(l->global_mode_).c_str(), tid);

			// if mode is the as restrictive as or more than the one allowed
			// by global mode then we need to communicate with the server.
			// otherwise we perform the conversion silently
			if (mode != l->global_mode_ &&
				lock_protocol::Mode::PartialOrder(mode, l->global_mode_) >= 0)  
			{
				if ((r = do_convert(l, mode)) == lock_protocol::OK) {
					l->global_mode_ = (lock_protocol::mode) mode;
				} else {
					// TODO: ISSUE cannot lock remotely. Modify server to 
					// allow releasing the lock voluntarily so that we 
					// can handle this as the NONE case
					assert(0); 
				}
			}
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
					"[%d] thread %lu got lock %llu at seq %d\n",
					cl2srv_->id(), tid, lid, l->seq_);
			l->gtque_.Add(ThreadRecord(tid, (lock_protocol::mode) mode));
			l->set_status(Lock::LOCKED);
			r = lock_protocol::OK;
			break;
		case Lock::ACQUIRING:
			// There is on-going lock acquisition; we just sit here and wait
			// its completion
			DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
			        "[%d] lck-%llu: another thread in acquisition\n",
			        cl2srv_->id(), lid);
			while (l->status() == Lock::ACQUIRING) {
				pthread_cond_wait(&l->status_cv_, &mutex_);
			}
			goto check_state;	// Status changed. Re-check.
		case Lock::LOCKED:
			if (l->gtque_.Exists(tid)) {
				// the current thread has already obtained the lock
				DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
				        "[%d] current thread already got lck %llu\n",
				        cl2srv_->id(), lid);
				r = lock_protocol::OK;
			} else {
				// we cannot lock at a more restrictive mode than the 
				// one allowed by the server 
				if (mode != l->global_mode_ &&
				    lock_protocol::Mode::PartialOrder(mode, l->global_mode_) < 0 && 
				    l->gtque_.CanGrant(mode)) 
				{
					DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
					        "[%d] thread %lu got lock %llu at seq %d\n",
							cl2srv_->id(), tid, lid, l->seq_);
					l->gtque_.Add(ThreadRecord(tid, (lock_protocol::mode) mode));
					r = lock_protocol::OK;
				} else {
					pthread_cond_wait(&l->status_cv_, &mutex_);
					goto check_state; // Status changed. Re-check.
				}
			}
			break;
		case Lock::NONE:
			dbg_log(DBG_INFO, "[%d] lock %llu not available; acquiring now\n",
			        cl2srv_->id(), lid);
			l->set_status(Lock::ACQUIRING);
			while ((r = do_acquire(l, mode)) == lock_protocol::RETRY) {
				while (!l->can_retry_) {
					pthread_cond_wait(&l->retry_cv_, &mutex_);
				}
			}
			if (r == lock_protocol::OK) {
				dbg_log(DBG_INFO, "[%d] thread %lu got lock %llu at seq %d\n",
				        cl2srv_->id(), tid, lid, l->seq_);
				l->global_mode_ = (lock_protocol::mode) mode;
				l->gtque_.Add(ThreadRecord(tid, (lock_protocol::mode) mode));
				l->set_status(Lock::LOCKED);
			}
			break;
		default:
			break;
	}
	return r;
}


lock_protocol::status
LockManager::Acquire(Lock* lock, int mode, int flags)
{
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	r = AcquireInternal(0, lock, mode, flags);
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status
LockManager::Acquire(lock_protocol::LockId lid, int mode, int flags)
{
	Lock*                 lock;
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	lock = GetOrCreateLockInternal(lid);
	r = AcquireInternal(0, lock, mode);
	pthread_mutex_unlock(&mutex_);
	return r;
}


inline lock_protocol::status
LockManager::ConvertInternal(unsigned long tid, Lock* l, int new_mode)
{
	lock_protocol::status r = lock_protocol::OK;
	lock_protocol::LockId lid = l->lid_;

	if (l->status() != Lock::LOCKED) {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] cannot convert non-locked lock %llu\n",
		        cl2srv_->id(), lid);
		r = lock_protocol::NOENT;
		return r;
	}

	if (l->gtque_.Exists(tid)) {
		// if new mode after conversion is as restrictive as or more than 
		// the one allowed by global mode then we need to communicate with the
		// server. otherwise we perform the conversion silently
		if (new_mode != l->global_mode_ &&
		    lock_protocol::Mode::PartialOrder(new_mode, l->global_mode_) >= 0)  
		{
			if ((r = do_convert(l, new_mode)) == lock_protocol::OK) {
				l->global_mode_ = (lock_protocol::mode) new_mode;
			} else {
				return r;
			}
		}
		assert(l->gtque_.ConvertInPlace(tid, new_mode) == 0);
		if (l->gtque_.Empty()) {
			l->set_status(Lock::FREE);
		} else {
			// signal status_cv_ because there is likely an internal mode change 
			// the releaser thread depends on
			pthread_cond_broadcast(&l->status_cv_);
		}
	} else { 
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] thread %lu is not holder of lck %llu\n",
		        cl2srv_->id(), tid, lid);
		r = lock_protocol::NOENT;
	}
	return r;
}


// release() is an atomic operation
lock_protocol::status
LockManager::Convert(Lock* lock, int mode)
{
	lock_protocol::status r;

	pthread_mutex_lock(&mutex_);
	r = ConvertInternal(0, lock, mode);
	pthread_mutex_unlock(&mutex_);
	return r;
}


// release() is an atomic operation
lock_protocol::status
LockManager::Convert(lock_protocol::LockId lid, int mode)
{
	lock_protocol::status r;
	Lock*                 lock;

	pthread_mutex_lock(&mutex_);
	assert(locks_.find(lid) != locks_.end());
	lock = GetOrCreateLockInternal(lid);
	r = ConvertInternal(0, lock, mode);
	pthread_mutex_unlock(&mutex_);
	return r;
}


inline lock_protocol::status
LockManager::ReleaseInternal(unsigned long tid, Lock* l)
{
	lock_protocol::status r = lock_protocol::OK;
	lock_protocol::LockId lid = l->lid_;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] Releasing lock %llu \n", cl2srv_->id(), lid); 

	if (l->gtque_.Exists(tid)) {
		l->gtque_.Remove(tid);
		if (l->gtque_.Empty()) {
			l->set_status(Lock::FREE);
		}
	} else { 
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "[%d] thread %lu is not holder of lck %llu\n",
		        cl2srv_->id(), tid, lid);
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
	r = ReleaseInternal(0, lock);
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
	assert(locks_.find(lid) != locks_.end());
	lock = GetOrCreateLockInternal(lid);
	r = ReleaseInternal(0, lock);
	pthread_mutex_unlock(&mutex_);
	return r;
}


rlock_protocol::status
LockManager::revoke(lock_protocol::LockId lid, int seq, int revoke_type, int &unused)
{
	rlock_protocol::status r = rlock_protocol::OK;
	Lock*                  l;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr),
	        "[%d] server request to revoke lck %llu at seq %d\n", 
	        cl2srv_->id(), lid, seq);

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


rlock_protocol::status
LockManager::retry(lock_protocol::LockId lid, int seq,
                   int& current_seq)
{
	rlock_protocol::status r = rlock_protocol::OK;
	Lock*                  l;

	pthread_mutex_lock(&mutex_);
	assert(locks_.find(lid) != locks_.end());
	l = GetOrCreateLockInternal(lid);

	if (seq >= l->seq_) {
		// it doesn't matter whether this retry message arrives before or
		// after the response to the corresponding acquire arrives, as long
		// as the sequence number of the retry matches that of the acquire
		assert(l->status() == Lock::ACQUIRING);
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
LockManager::do_acquire(Lock* l, int mode)
{
	lock_protocol::rpc_numbers rpc_number;
	int                        r;
	int                        unused;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] calling acquire rpc for lck %llu id=%d seq=%d\n",
	        cl2srv_->id(), l->lid_, cl2srv_->id(), last_seq_+1);
	r = cl2srv_->call(lock_protocol::acquire, cl2srv_->id(), ++last_seq_, l->lid_, 
	                  mode, 0, unused);
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
LockManager::do_convert(Lock* l, int mode)
{
	int r;
	int unused;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
	        "[%d] calling convert rpc for lck %llu id=%d seq=%d\n",
	        cl2srv_->id(), l->lid_, cl2srv_->id(), l->seq_);
	if (lu_) {
		lu_->OnConvert(l);
	}
	r = cl2srv_->call(lock_protocol::convert, cl2srv_->id(), l->seq_, l->lid_, 
	                  mode, unused);
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
	if (lu_) {
		lu_->OnRelease(l);
	}
	r = cl2srv_->call(lock_protocol::release, cl2srv_->id(), l->seq_, l->lid_, 
	                  unused);
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
