/// \file lckmgr.h 
///
/// \brief Client lock manager interface.

#ifndef _CLIENT_LOCK_MANAGER_H_AFG819
#define _CLIENT_LOCK_MANAGER_H_AFG819

#include <string>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include <set>
#include "rpc/rpc.h"
#include "common/gtque.h"
#include "common/lock_protocol.h"

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



class ThreadRecord {
public:
	typedef pthread_t id_t;
	typedef lock_protocol::mode mode_t;
	typedef lock_protocol::Mode Mode;

	ThreadRecord();
	ThreadRecord(id_t, mode_t);

	id_t id() const { return tid_; };
	mode_t mode() const { return mode_; };
	void set_mode(mode_t mode) { mode_ = mode; };
	void set_mode(int mode) { mode_ = (mode_t) mode; };

private:
	id_t   tid_;
	mode_t mode_;
};


class Lock {

public:
	enum LockStatus {
		NONE, 
		FREE, 
		LOCKED, 
		ACQUIRING, 
		/* RELEASING (unused) */
	};

	enum Mode {
		NL = lock_protocol::NL,     // not locked
		SL = lock_protocol::SL,     // shared local
		SR = lock_protocol::SR,     // shared recursive
		IS = lock_protocol::IS,     // intent shared
		IX = lock_protocol::IX,     // intent exclusive
		XL = lock_protocol::XL,     // exclusive local
		XR = lock_protocol::XR,     // exclusive recursive
		IXSL = lock_protocol::IXSL, // intent exclusive and shared local
	};

	enum Flag {
		FLG_NOBLK = 0x1, // don't block if can't grant lock
	};

	Lock(lock_protocol::LockId);
	~Lock();

	// changes the status of this lock
	void set_status(LockStatus);
	LockStatus status() const { return status_; }
	lock_protocol::LockId lid() const { return lid_; }
	lock_protocol::mode mode(pthread_t tid) { 
		ThreadRecord t = gtque_.Find(tid);
		return t.mode();
	}

	lock_protocol::LockId lid_;

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

	lock_protocol::mode       global_mode_; ///< mode as known by the server

	void*                     payload_;     ///< lock users may use it for anything they like

private:
	LockStatus                status_;
};


// Classes that inherit LockUser can override callback functions to 
// be notified of lock events
class LockUser {
public:
	virtual void OnRelease(Lock*) = 0;
	virtual void OnConvert(Lock*) = 0;
	virtual int Revoke(Lock*) = 0;
	virtual ~LockUser() {};
};


class LockManager {
public:
	LockManager(rpcc* rpc_client, rpcs* rpc_server, std::string id, class LockUser* lu);
	~LockManager();
	Lock* FindOrCreateLock(lock_protocol::LockId lid);
	lock_protocol::status Acquire(Lock* lock, int mode, int flags, std::vector<unsigned long long> argv);
	lock_protocol::status Acquire(Lock* lock, int mode, int flags);
	lock_protocol::status Acquire(lock_protocol::LockId lid, int mode, int flags, std::vector<unsigned long long> argv);
	lock_protocol::status Acquire(lock_protocol::LockId lid, int mode, int flags);
	lock_protocol::status Convert(Lock* lock, int new_mode);
	lock_protocol::status Convert(lock_protocol::LockId lid, int new_mode);
	lock_protocol::status Release(Lock* lock);
	lock_protocol::status Release(lock_protocol::LockId lid);
	lock_protocol::status stat(lock_protocol::LockId lid);
	void Releaser();
	void RegisterLockUser(LockUser* lu) { lu_ = lu; };

	rlock_protocol::status revoke(lock_protocol::LockId lid, int seq, int revoke_type, int& unused);
	// Tell this client to retry requesting the lock in which this client
	// was interest when that lock just became available.
	rlock_protocol::status retry(lock_protocol::LockId lid, int seq, int& current_seq);

	int id() { return cl2srv_->id(); }

private:
	int do_acquire(Lock* l, int mode, int flags, std::vector<unsigned long long> argv);
	int do_convert(Lock* l, int mode, int flags);
	int do_release(Lock* l);
	Lock* FindOrCreateLockInternal(lock_protocol::LockId lid);
	lock_protocol::status AcquireInternal(unsigned long tid, Lock* l, int mode, int flags, std::vector<unsigned long long> argv);
	lock_protocol::status ConvertInternal(unsigned long tid, Lock* l, int new_mode);
	lock_protocol::status ReleaseInternal(unsigned long tid, Lock* e);

	class LockUser*                                      lu_;
	std::string                                          hostname_;
	std::string                                          id_;
	/// the RPC object through which we receive callbacks from the server
	rpcs*                                                srv2cl_;
	/// the RPC object through which we make calls to the server
	rpcc*                                                cl2srv_;

	int                                                  last_seq_;
	bool                                                 running_;

	google::dense_hash_map<lock_protocol::LockId, Lock*> locks_;

	// key: lock id; value: seq no. of the corresponding acquire
	std::map<lock_protocol::LockId, int>                 revoke_map_;
	// global lock
	pthread_mutex_t                                      mutex_;
	// controls access to the revoke_map
	pthread_mutex_t                                      revoke_mutex_;
	pthread_cond_t                                       revoke_cv;
};


} // namespace client

#endif  // _CLIENT_LOCK_MANAGER_H_AFG819
