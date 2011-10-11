/// \file hlckmgr.h 
///
/// \brief Client hierarchical lock manager interface.

#ifndef _CLIENT_HIERARCHICAL_LOCK_MANAGER_H_ABN145
#define _CLIENT_HIERARCHICAL_LOCK_MANAGER_H_ABN145

#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include <google/dense_hash_set>
#include "client/lckmgr.h"

namespace client {

class HLock;
typedef google::dense_hash_set<HLock*> HLockPtrSet;

/// The hierarchical lock keeps the locking mode to be able to acquire 
/// a base lock if it needs to (e.g. when the parent's base lock is 
/// released or downgraded). 
/// To simplify the implementation of the hierarchical lock and to make 
/// it more lightweight, the lock cannnot be acquired by multiple threads
/// concurrently. Thus, the locking mode is not used to synchronize threads
class HLock {
public:
	enum LockStatus {
		NONE              = 0x0, 
		FREE              = 0x1, 
		LOCKED            = 0x2,  
		ACQUIRING         = 0x4, 
		CONVERTING        = 0x8,
		LOCKED_CONVERTING = 0xA 
	};
	
	enum Flag {
		FLG_PUBLIC = 0x1, // acquire a globally visible lock 
	};


	HLock(lock_protocol::LockId, HLock*);
	HLock(HLock*);
	
	// changes the status of this lock
	void set_status(LockStatus);
	LockStatus status() const { return status_; }
	lock_protocol::LockId lid() const { return lid_; }
	int AddChild(HLock* hlock);
	int ChangeStatus(LockStatus old_sts, LockStatus new_sts);
	int WaitStatus2(LockStatus old_sts1, LockStatus old_sts2);
	int BeginConverting(bool lock);
	int EndConverting(bool lock);
	

	Lock*                 lock_;
	HLock*                parent_;
	
	pthread_t             owner_; ///< thread that owns the lock
	/// we use only a single cv to monitor changes of the lock's status
	/// this may be less efficient because there would be many spurious
	/// wake-ups, but it's simple anyway
	pthread_cond_t        status_cv_;
	
	/// condvar that is signaled when the ``used'' field is set to true
	pthread_cond_t        used_cv_;

	pthread_mutex_t       mutex_;
	bool                  used_;                    ///< set to true after first use
	bool                  can_retry_;               ///< set when a retry message from the server is received
	/// private locking mode. indicates the mode locked by the hierarchical 
	/// manager. this is the mode seen by the local user of the lock. 
	/// invariant: mode_ is collective (i.e. it is never reduced). it indicates 
	///  the most severe mode this lock has ever been locked at during its lifetime
	///  (i.e. till lock is publicly released and is returned to status NONE)
	lock_protocol::Mode   mode_;                    
	lock_protocol::Mode   ancestor_recursive_mode_; ///< recursive mode of ancestors
	lock_protocol::LockId lid_;
	HLockPtrSet           children_;
private:	
	LockStatus            status_;
};



class HLockUser {
public:
	virtual void OnRelease(HLock*) = 0;
	virtual void OnConvert(HLock*) = 0;
	virtual int Revoke(HLock*) = 0;
	virtual ~HLockUser() {};
};


class HLockManager: public LockUser {
public:
	enum Status {
		NONE = 0,
		ATTACHING=1,
		REVOKING
	};

	enum Flag {
		FLG_HAVEPARENT = 1
	};

	HLockManager(LockManager*, HLockUser* = 0);
	
	void OnRelease(Lock* l) { return; };
	void OnConvert(Lock* l) { return; };
	int Revoke(Lock* lock, lock_protocol::Mode mode);

	HLock* FindOrCreateLock(lock_protocol::LockId lid, lock_protocol::LockId plid);

	lock_protocol::status Acquire(HLock* hlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status Acquire(lock_protocol::LockId lid, lock_protocol::LockId, lock_protocol::Mode mode, int flags);
	lock_protocol::status Release(HLock* hlock);
	lock_protocol::status Release(lock_protocol::LockId lid);

private:
	HLock* FindLockInternal(lock_protocol::LockId lid, HLock* plp);
	HLock* FindOrCreateLockInternal(lock_protocol::LockId lid, HLock* plp);
	lock_protocol::status PropagatePublicLock(HLock* phlock, HLock* chlock, lock_protocol::Mode mode);
	lock_protocol::status AttachPublicLock(HLock* hlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status AttachPublicLockChainUp(HLock* hlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status AttachPublicLockChildren(HLock* hlock, lock_protocol::Mode mode);
	lock_protocol::status AttachPublicLockChild(HLock* phlock, HLock* chlock, lock_protocol::Mode mode);
	lock_protocol::status AttachPublicLockCapability(HLock* hlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status AcquireInternal(pthread_t tid, HLock* hlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status ReleaseInternal(pthread_t tid, HLock* hlock);

	pthread_mutex_t      mutex_;
	Status               status_;
	pthread_cond_t       status_cv_;
	HLockUser*           hlu_;
	LockManager*         lm_;
	google::dense_hash_map<lock_protocol::LockId, HLock*> locks_;
};


inline int 
HLock::AddChild(HLock* hlock)
{
	children_.insert(hlock);
}



} // namespace client

#endif /* _CLIENT_HIERARCHICAL_LOCK_MANAGER_H_ABN145 */
