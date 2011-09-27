/// \file hlckmgr.h 
///
/// \brief Client hierarchical lock manager interface.

#ifndef _CLIENT_HIERARCHICAL_LOCK_MANAGER_H_ABN145
#define _CLIENT_HIERARCHICAL_LOCK_MANAGER_H_ABN145

#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "client/lckmgr.h"

namespace client {


// invariant: mode < root.mode
class HLock {
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

	HLock(lock_protocol::LockId, HLock*);
	HLock(HLock*);

	Lock*                 lock_;
	HLock*                parent_;
	pthread_mutex_t       mutex_;
	int                   status_;
	int                   supremum_mode_; // supremum of all ancestors' modes
	int                   mode_; // local mode
	lock_protocol::LockId lid_;
	
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
	HLockManager(LockManager*, HLockUser* = 0);
	
	void OnRelease(Lock*) { return; };
	void OnConvert(Lock*) { return; };
	int Revoke(Lock*);

	HLock* FindOrCreateLockInternal(lock_protocol::LockId);
	HLock* FindOrCreateLock(lock_protocol::LockId lid);
	HLock* InitLock(lock_protocol::LockId, lock_protocol::LockId);
	HLock* InitLock(lock_protocol::LockId, HLock*);
	HLock* InitLock(HLock*, HLock*);

	lock_protocol::status Acquire(HLock* hlock, int mode, int flags);
	lock_protocol::status Acquire(lock_protocol::LockId lid, int mode, int flags);

private:
	pthread_mutex_t      mutex_;
	HLockUser*           hlu_;
	LockManager*         lm_;
	google::dense_hash_map<lock_protocol::LockId, HLock*> locks_;
};


} // namespace client

#endif /* _CLIENT_HIERARCHICAL_LOCK_MANAGER_H_ABN145 */
