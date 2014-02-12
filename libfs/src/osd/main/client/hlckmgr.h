/// \file hlckmgr.h 
///
/// \brief Client hierarchical lock manager interface.

#ifndef __STAMNOS_OSD_CLIENT_HIERARCHICAL_LOCK_MANAGER_H
#define __STAMNOS_OSD_CLIENT_HIERARCHICAL_LOCK_MANAGER_H

#include <utility>
#include <google/dense_hash_map>
#include <google/dense_hash_set>
#include "osd/main/client/lckmgr.h"

namespace osd {
namespace cc {
namespace client {

class HLock; // forward declaration

struct HLockPtrLockModePair {
	HLockPtrLockModePair(HLock* hlock, lock_protocol::Mode mode)
		: hlock_(hlock),
		  mode_(mode)
	{ }
	HLock*              hlock_;
	lock_protocol::Mode mode_;
};

typedef std::list<HLockPtrLockModePair> HLockPtrLockModePairSet;
typedef std::list<HLock*> HLockPtrList;
typedef google::dense_hash_set<HLock*> HLockPtrSet;
/**
 * A hierarchical lock is identified by an ID=(TYPE_ID, NUMBER)
 * Don't confuse the TYPE_ID of the hierarchical lock with the TYPE_ID
 * of the underlying base lock. The base lock TYPE_ID for the hierarchical
 * lock is HLock::TypeId.
 * 
 * The hierarchical lock keeps the locking mode to be able to acquire 
 * a base lock if it needs to (e.g. when the parent's base lock is 
 * released or downgraded). 
 * To simplify the implementation of the hierarchical lock and to make 
 * it more lightweight, the lock cannnot be acquired by multiple threads
 * concurrently. Thus, the locking mode is not used to synchronize threads
 * insight : but can the hlock be acquired by multiple procs at the same time ?
 */
class HLock {
public:
	enum {
		TypeId = 1 
	};

	enum LockStatus {
		NONE              = 0x0, 
		FREE              = 0x1, 
		LOCKED            = 0x2,  
		ACQUIRING         = 0x4, 
		CONVERTING        = 0x8,
		LOCKED_CONVERTING = 0xA 
	};
	
	enum Flag {
		FLG_NOBLK = lock_protocol::FLG_NOQUE, 
		FLG_CAPABILITY = lock_protocol::FLG_CAPABILITY,
		FLG_PUBLIC = 0x4, // acquire a globally visible lock 
		FLG_STICKY = 0x8
	};


	HLock(LockId lid);
	
	// changes the status of this lock
	void set_status(LockStatus);
	LockStatus status() const { return status_; }
	LockId lid() const { return lid_; }
	LockId base_lid() const { return LockId(HLock::TypeId, lid_.number()); }
	int AddChild(HLock* hlock);
	int ChangeStatus(LockStatus old_sts, LockStatus new_sts);
	int WaitStatus(LockStatus old_sts);
	int WaitStatus2(LockStatus old_sts1, LockStatus old_sts2);
	int BeginConverting(bool lock);
	int EndConverting(bool lock);

	void* payload() { return payload_; }
	void set_payload(void* payload) { payload_ = payload; }

	Lock*                 lock_;   //!< the public base lock if one is attached
	//insight : What is this public base lock ??
	HLock*                parent_;
	
	pthread_t             owner_;  //!< thread that owns the lock
	//! we use only a single cv to monitor changes of the lock's status
	//! this may be less efficient because there would be many spurious
	//! wake-ups, but it's simple anyway
	pthread_cond_t        status_cv_;
	
	/// condvar that is signaled when the ``used'' field is set to true
	pthread_cond_t        used_cv_;

	pthread_mutex_t       mutex_;
	bool                  sticky_;                  ///< hint: user will reacquire the lock after the lock is revoked. 
	bool                  used_;                    ///< set to true after first use
	bool                  can_retry_;               ///< set when a retry message from the server is received
	/// private locking mode. indicates the mode locked by the hierarchical 
	/// manager. this is the mode seen by the local user of the lock. 
	/// invariant: mode_ is collective (i.e. it is never reduced). it indicates 
	///  the most severe mode this lock has ever been locked at during its lifetime
	///  (i.e. till lock is publicly released and is returned to status NONE)
	lock_protocol::Mode   mode_;                    
	lock_protocol::Mode   ancestor_recursive_mode_; ///< recursive mode of ancestors
	LockId                lid_;
	HLockPtrList          children_;
	int                   owner_depth_; ///< how many times the lock is locked by owner   
	void*                 payload_;     ///< lock users may use it for anything they like
private:	
	LockStatus            status_;
};


class HLockCallback {
public:
	virtual void OnRelease(HLock*) = 0;
	virtual void OnConvert(HLock*) = 0;
	virtual void PreDowngrade() = 0;
	virtual ~HLockCallback() {};
};


class HLockManager: public LockRevoke {
	typedef google::dense_hash_map<LockId, HLock*, LockIdHashFcn> LockMap;
public:
	enum Status {
		NONE = 0,
		ATTACHING = 1,
		REVOKING,
		SHUTTING_DOWN
	};

	HLockManager(LockManager*);
	~HLockManager();
	
	int Revoke(Lock* lock, lock_protocol::Mode mode);

	HLock* FindOrCreateLock(LockId lid);

	lock_protocol::status Acquire(HLock* hlock, HLock* phlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status Acquire(HLock* hlock, lock_protocol::Mode mode, int flags);

	lock_protocol::status Acquire(LockId lid, HLock* phlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status Acquire(LockId lid, LockId, lock_protocol::Mode mode, int flags);
	lock_protocol::status Acquire(LockId lid, lock_protocol::Mode mode, int flags);
	
	lock_protocol::status Release(HLock* hlock);
	lock_protocol::status Release(LockId lid);
	void RegisterLockCallback(HLockCallback* hcb) { hcb_ = hcb; };
	void UnregisterLockCallback() { hcb_ = NULL; };

	int LockChain(HLock* hlock, LockId lckarray[]);
	void PrintDebugInfo();
	unsigned int id() { return lm_->id(); }

private:
	HLock* FindLockInternal(LockId lid);
	HLock* FindOrCreateLockInternal(LockId lid);
	lock_protocol::status AttachPublicLock(HLock* hlock, lock_protocol::Mode mode, bool caller_has_parent, int flags);
	lock_protocol::status AttachPublicLockChainUp(HLock* hlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status AttachPublicLockToChildren(HLock* hlock, lock_protocol::Mode mode);
	lock_protocol::status AttachPublicLockToChild(HLock* phlock, HLock* chlock, lock_protocol::Mode mode);
	lock_protocol::status AttachPublicLockCapability(HLock* hlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status AcquireInternal(pthread_t tid, HLock* hlock, HLock* phlock, lock_protocol::Mode mode, int flags);
	lock_protocol::status ReleaseInternal(pthread_t tid, HLock* hlock, bool force);
	lock_protocol::status DowngradePublicLock(HLock* hlock, lock_protocol::Mode new_mode);
	lock_protocol::status DowngradePublicLockRecursive(HLock* hlock, lock_protocol::Mode new_public_mode, HLockPtrLockModePairSet* release_set);

	pthread_mutex_t      mutex_;
	Status               status_;
	pthread_cond_t       status_cv_;
	HLockCallback*       hcb_;
	LockManager*         lm_;
	LockMap              locks_;
};


inline int 
HLock::AddChild(HLock* hlock)
{
	children_.push_back(hlock);
	return E_SUCCESS;
}


} // namespace client
} // namespace cc
} // namespace osd


namespace client {
	typedef ::osd::cc::client::HLock         HLock;
	typedef ::osd::cc::client::HLockManager  HLockManager;
	typedef ::osd::cc::client::HLockCallback HLockCallback;
} // namespace client



#endif // __STAMNOS_OSD_CLIENT_HIERARCHICAL_LOCK_MANAGER_H
