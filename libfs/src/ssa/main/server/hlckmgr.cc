#include "ssa/main/server/hlckmgr.h"
#include "ipc/ipc.h"
#include "common/debug.h"
#include "ssa/main/common/cc.h"
#include "ssa/main/common/lock_protocol.h"
#include "ssa/main/server/lckmgr.h"

namespace ssa {
namespace cc {
namespace server {

/**
 * \todo Modify base lock manager to allocate locks as objects and 
 *       use google's hash map
 * \todo Extent base lock manager to provide an API for use by this 
 *       server which accepts a pointer to a object collect all RPC 
 *       under subclass RPC
 */

typedef ssa::cc::common::LockId LockId;

HLockManager::HLockManager(::server::Ipc* ipc)
	: ipc_(ipc),
	  lm_(NULL)
{ }


int
HLockManager::Init()
{
	dbg_log(DBG_INFO, "Initializing Hierachical Lock Manager\n");

	pthread_mutex_init(&mutex_, NULL);

	lm_ = new LockManager(ipc_, false, &mutex_);

	if (ipc_) {
		ipc_->reg(lock_protocol::stat, this, &HLockManager::Stat);
		ipc_->reg(lock_protocol::acquire, this, &HLockManager::Acquire);
		ipc_->reg(lock_protocol::release, this, &HLockManager::Release);
		ipc_->reg(lock_protocol::convert, this, &HLockManager::Convert);
	}
}


HLockManager::~HLockManager()
{
	delete lm_;
}


lock_protocol::status
HLockManager::Acquire(int clt, int seq, lock_protocol::LockId lid, 
                      int mode_seti, int flags, 
                      unsigned long long arg, int& mode_granted)
{
	lock_protocol::status              r;
	lock_protocol::LockId              plid = (lock_protocol::LockId) arg;
	Lock*                              lock;
	Lock*                              plock;
	lock_protocol::Mode                plock_mode = lock_protocol::Mode::NL;
	lock_protocol::Mode::Set           mode_set = lock_protocol::Mode::Set(mode_seti);
	lock_protocol::Mode::Set::Iterator itr;
	ClientRecord*                      cr;

	dbg_log(DBG_INFO, "clt %d seq %d acquiring hierarchical lock %s (%s)\n",
	        clt, seq, LockId(lid).c_str(), mode_set.String().c_str());

	// just pass the lock request to the base lock manager
	if (LockId(lid).type() == 0) {
		return lm_->Acquire(clt, seq, lid, mode_seti, flags, arg, mode_granted);
	}
			

	pthread_mutex_lock(&mutex_);
	lock = lm_->FindOrCreateLockInternal(lid);

	if (flags & lock_protocol::FLG_CAPABILITY) {
		dbg_log(DBG_INFO, "clt %d seq %d acquiring hierarchical lock %s (%s)"
					" using capability\n", clt, seq, LockId(lid).c_str(), 
					mode_set.String().c_str());
		//TODO verify capability or get capability from the table
	} else {
		if ((plock = lm_->FindOrCreateLockInternal(plid)) &&
			(cr = plock->gtque_.Find(clt))) 
		{
			plock_mode = cr->mode();
		} else {
			r = lock_protocol::HRERR;
			goto done;
		}
		dbg_log(DBG_INFO, "clt %d seq %d acquiring hierarchical lock %s (%s)"
				" under hierarchical lock %s (%s)\n", clt, seq, LockId(lid).c_str(), 
				mode_set.String().c_str(), LockId(plid).c_str(), plock_mode.String().c_str());
		for (itr = mode_set.begin(); itr != mode_set.end(); itr++) {
			if (!lock_protocol::Mode::AbidesHierarchyRule((*itr), plock_mode)) {
				mode_set.Remove((*itr));
			}
		}
		if (mode_set.Empty()) {
			r = lock_protocol::HRERR;
			goto done;
		}
	}
	r = lm_->AcquireInternal(clt, seq, lock, lock_protocol::Mode::Set(mode_set), 
							 flags, mode_granted);
done:
	pthread_mutex_unlock(&mutex_);
	return r;
}


// convert does not block to avoid any deadlocks.
lock_protocol::status
HLockManager::Convert(int clt, int seq, lock_protocol::LockId lid, 
                      int new_mode, int flags, int& unused)
{
	return lm_->Convert(clt, seq, lid, new_mode, flags, unused);
}


lock_protocol::status
HLockManager::Release(int clt, int seq, lock_protocol::LockId lid, int flags, int& unused)
{
	return lm_->Release(clt, seq, lid, flags, unused);
}


lock_protocol::status
HLockManager::Stat(lock_protocol::LockId lid, int& r)
{
	return lm_->Stat(lid, r);
}


/*
HLockManager::CreateCapability(Lock*) 
{

}

lock_protocol::status
HLockManager::Acquire(int clt, Lock& lock, lock_protocol::Mode mode, int flags, int argc, void** argv) 
{
	lock_protocol::status r = lock_protocol::OK;
	lock_protocol::LockId plid = (lock_protocol::LockId) (argv[0]);

	if (flags & FLG_CAPABILITY) {
		
	}

	return r;
}


HLockManager::OnRelease(Lock* lock, int flags)
{
	if (flags & FLG_CAPABILITY) {
		
	}
}

*/

} // namespace server
} // namespace cc
} // namespace ssa
