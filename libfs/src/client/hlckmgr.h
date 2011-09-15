/// \file hlckmgr.h 
///
/// \brief Client hierarchical lock manager interface.

#ifndef _CLIENT_HIERARCHICAL_LOCK_MANAGER_H_ABN145
#define _CLIENT_HIERARCHICAL_LOCK_MANAGER_H_ABN145

#include "client/lckmgr.h"

namespace client {


// invariant: mode < root.mode
class HLock {
public:
	HLock(lock_protocol::LockId);
	HLock(HLock*);

	Lock*                 lock_;
	HLock*                parent_;
	pthread_mutex_t       mutex_;
	int                   status_;
	int                   mode_;
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

private:
	HLockUser*           hlu_;
	LockManager*         lm_;
};


} // namespace client

#endif /* _CLIENT_HIERARCHICAL_LOCK_MANAGER_H_ABN145 */
