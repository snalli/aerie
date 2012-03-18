/// \file lckmgr.h 
///
/// \brief Client lock manager interface.

#ifndef __STAMNOS_SSA_CLIENT_LOCK_MANAGER_H
#define __STAMNOS_SSA_CLIENT_LOCK_MANAGER_H

#include <string>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include <google/dense_hash_set>
#include <set>
#include "bcs/bcs.h"
#include "ssa/main/common/cc.h"
#include "ssa/main/common/gtque.h"
#include "common/bitmap.h"

namespace ssa {
namespace cc {
namespace client {


// SUGGESTED LOCK CACHING IMPLEMENTATION PLAN:
//
// All the requests on the server run till completion and threads wait 
// on condition variables on the client to wait for a lock. This allows 
// the server to:
//  - be replicated using the replicated state machine approach (Schneider 1990)
//  - serve requests using a single core in an event-based fashion
//
// On the client a lock can be in several states:
//  - free: client owns the lock (exclusively) and no thread has it
//  - locked: client owns the lock (exclusively) and a thread has it
//  - acquiring: the client is acquiring lock
//  - releasing: the client is releasing ownership
//
// in the state acquiring and xlocked there may be several threads
// waiting for the lock, but the first thread in the list interacts
// with the server and wakes up the threads when its done (released
// the lock).  a thread in the list is identified by its thread id
// (tid).
//
// a thread is in charge of getting a lock: if the server cannot grant
// it the lock, the thread will receive a retry reply.  at some point
// later, the server sends the thread a retry RPC, encouraging the client
// thread to ask for the lock again.
//
// once a thread has acquired a lock, its client obtains ownership of
// the lock. the client can grant the lock to other threads on the client 
// without interacting with the server. 
//
// the server must send the client a revoke request to get the lock back. this
// request tells the client to send the lock back to the
// server when the lock is released or right now if no thread on the
// client is holding the lock.  when receiving a revoke request, the
// client adds it to a list and wakes up a releaser thread, which returns
// the lock the server as soon it is free.
//
// the releasing is done in a separate a thread to avoid
// deadlocks and to ensure that revoke and retry RPCs from the server
// run to completion (i.e., the revoke RPC cannot do the release when
// the lock is free.
//
// a challenge in the implementation is that retry and revoke requests
// can be out of order with the acquire and release requests. That
// is, a client may receive a revoke request before it has received
// the positive acknowledgement on its acquire request.  Similarly, a
// client may receive a retry before it has received a response on its
// initial acquire request.  A flag field is used to record if a retry
// has been received.
//


// ISSUES to be filed in the issue tracker
//
// include extra data structure outstanding_locks_to keep track of outstanding lock 
// requests to speeden up lookup of outstanind locks. the downside of including 
// such a data structure is that it is placed in the critical path of lock acquisition
// (but since lock acquisition involves an RPC, we expect the overhead of the data
// structure to be relatively small)
//
// 
// currently we serialize access to the lock state through the lock manager global
// lock. we expect this to be okay as public locks are heavyweight (involve RPC)
// consider using fine grain mutex (one per lock) if we see contention.

typedef ssa::cc::common::LockType       LockType;
typedef ssa::cc::common::LockId         LockId;
typedef ssa::cc::common::LockIdHashFcn  LockIdHashFcn;

class ThreadRecord {
public:
	typedef pthread_t            id_t;
	typedef lock_protocol::Mode  Mode;

	ThreadRecord();
	ThreadRecord(id_t, Mode);

	id_t id() const { return tid_; };
	Mode mode() const { return mode_; };
	void set_mode(Mode mode) { mode_ = mode; };

private:
	id_t tid_;
	Mode mode_;
};


class Lock {

public:
	enum {
		TypeId = 0 
	};
	
	enum LockStatus {
		NONE, 
		FREE, 
		LOCKED, 
		ACQUIRING, 
		/* RELEASING (unused) */
	};

	enum Flag {
		FLG_NOBLK = lock_protocol::FLG_NOQUE, // don't block if can't grant lock
		FLG_CAPABILITY = lock_protocol::FLG_CAPABILITY
	};

	Lock(LockId);
	~Lock();

	// changes the status of this lock
	void set_status(LockStatus);
	LockStatus status() const { return status_; }
	LockId lid() const { return lid_; }
	lock_protocol::Mode mode(pthread_t tid) { 
		ThreadRecord* t = gtque_.Find(tid);
		return (t != NULL) ? t->mode(): lock_protocol::Mode(lock_protocol::Mode::NL);
	}
	
	void* payload() { return payload_; }
	void set_payload(void* payload) { payload_ = payload; }

	LockId                    lid_;

	/// we use only a single cv to monitor changes of the lock's status
	/// this may be less efficient because there would be many spurious
	/// wake-ups, but it's simple anyway
	pthread_cond_t            status_cv_;

	/// condvar that is signaled when the ``used'' field is set to true
	pthread_cond_t            used_cv_;

	pthread_cond_t            got_acq_reply_cv_;
  
	/// condvar that is signaled when the server informs the client to retry
	pthread_cond_t            retry_cv_;

	GrantQueue<ThreadRecord>  gtque_; 
	
	/// The sequence number of the latest *COMPLETED* acquire request made
	/// by the client to obtain this lock.
	/// By completed, we mean the remote acquire() call returns with a value.
	int                       seq_;
	bool                      used_;        ///< set to true after first use
	bool                      can_retry_;   ///< set when a retry message from the server is received
	int                       revoke_type_; ///< type of revocation requested
	lock_protocol::Mode       public_mode_; ///< mode as known by the server and seen by the world
	void*                     payload_;     ///< lock users may use it for anything they like
	bool                      cancel_;      ///< cancel outstanding request
	
private:
	LockStatus                status_;
};


// Classes that inherit LockCallback can override callback functions to 
// be notified of lock events
class LockCallback {
public:
	virtual void OnRelease(Lock*) = 0;
	virtual void OnConvert(Lock*) = 0;
	virtual ~LockCallback() {};
};


// Classes that inherit LockRevoke can specialize the revoke function to 
// take appropriate action
class LockRevoke {
public:
	virtual int Revoke(Lock* lock, lock_protocol::Mode mode) = 0;
	virtual ~LockRevoke() {};
};


class LockManager {
	enum {
		LOCK_TYPE_COUNT = 2
	};
	typedef google::dense_hash_map<LockId, Lock*, LockIdHashFcn> LockMap;
	typedef google::dense_hash_map<LockId, int, LockIdHashFcn>  RevokeMap;
public:
	LockManager(::client::Ipc* ipc);
	~LockManager();
	int Init();
	Lock* FindLock(LockId lid);
	Lock* FindOrCreateLock(LockId lid);
	lock_protocol::status Acquire(Lock* lock, lock_protocol::Mode::Set mode_set, int flags, int argc, void** argv, lock_protocol::Mode& mode_granted);
	lock_protocol::status Acquire(Lock* lock, lock_protocol::Mode::Set mode_set, int flags, lock_protocol::Mode& mode_granted);
	lock_protocol::status Acquire(LockId lid, lock_protocol::Mode::Set mode_set, int flags, int argc, void** argv, lock_protocol::Mode& mode_granted);
	lock_protocol::status Acquire(LockId lid, lock_protocol::Mode::Set mode_set, int flags, lock_protocol::Mode& mode_granted);
	lock_protocol::status Convert(Lock* lock, lock_protocol::Mode new_mode, bool synchronous = false);
	lock_protocol::status Convert(LockId lid, lock_protocol::Mode new_mode, bool synchronous = false);
	lock_protocol::status Release(Lock* lock, bool synchronous = false);
	lock_protocol::status Release(LockId lid, bool synchronous = false);
	lock_protocol::status Cancel(Lock* l);
	lock_protocol::status Cancel(LockId lid);
	lock_protocol::status stat(LockId lid);
	void Releaser();
	void ShutdownReleaser();
	void RegisterLockCallback(LockType type, LockCallback* lu);
	void RegisterLockRevoke(LockType type, LockRevoke* lu);
	void UnregisterLockCallback(LockType type);
	void UnregisterLockRevoke(LockType type);

	rlock_protocol::status revoke(lock_protocol::LockId, int seq, int revoke_type, int& unused);
	rlock_protocol::status retry(lock_protocol::LockId, int seq, int& current_seq);

	unsigned int id() { return ipc_->id(); }

private:
	int do_acquire(Lock* l, lock_protocol::Mode::Set mode_set, int flags, int argc, void** argv, lock_protocol::Mode& mode_granted);
	int do_convert(Lock* l, lock_protocol::Mode mode, int flags);
	int do_release(Lock* l, int flags);
	Lock* FindLockInternal(LockId lid);
	Lock* FindOrCreateLockInternal(LockId lid);
	lock_protocol::status AcquireInternal(unsigned long tid, Lock* l, lock_protocol::Mode::Set mode_set, int flags, int argc, void** argv, lock_protocol::Mode& mode_granted);
	lock_protocol::status ConvertInternal(unsigned long tid, Lock* l, lock_protocol::Mode new_mode, bool synchronous);
	lock_protocol::status ReleaseInternal(unsigned long tid, Lock* e, bool synchronous);
	lock_protocol::Mode SelectMode(Lock* l, lock_protocol::Mode::Set mode_set);
	lock_protocol::status CancelLockRequestInternal(Lock* l);

	class LockCallback*     lcb_[LOCK_TYPE_COUNT];
	class LockRevoke*       lrvk_[LOCK_TYPE_COUNT];
	
	::client::Ipc*          ipc_;
	int                     last_seq_;
	volatile bool           releaser_thread_running_;

	/// locks known to this lock manager
	LockMap                 locks_;

	// global lock
	pthread_mutex_t         mutex_;
	// key: lock id; value: seq no. of the corresponding acquire
	RevokeMap               revoke_map_;
	// controls access to the revoke_map
	pthread_mutex_t         revoke_mutex_;
	pthread_cond_t          revoke_cv;
	pthread_t               releasethread_th_;
};


} // namespace client
} // namespace cc
} // namespace ssa


namespace client {
	typedef ::ssa::cc::client::Lock        Lock;
	typedef ::ssa::cc::client::LockManager LockManager;
} // namespace client

#endif // __STAMNOS_SSA_CLIENT_LOCK_MANAGER_H
