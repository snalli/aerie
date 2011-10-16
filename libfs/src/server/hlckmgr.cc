#include "server/hlckmgr.h"
#include "rpc/rpc.h"
#include "common/lock_protocol.h"
#include "server/lckmgr.h"

namespace server {

//TODO: modify base lock manager to allocate locks as objects and use google's hash map
//extent base lock manager to provide an API for use by this server which accepts a pointer to a object
//collect all RPC under subclass RPC


HLockManager::HLockManager(rpcs* rpc_server)
{
	lm_ = new LockManager();

	if (rpc_server) {
		rpc_server->reg(lock_protocol::stat, this, &HLockManager::Stat);
		rpc_server->reg(lock_protocol::acquire, this, &HLockManager::Acquire);
		rpc_server->reg(lock_protocol::acquirev, this, &HLockManager::AcquireVector);
		rpc_server->reg(lock_protocol::release, this, &HLockManager::Release);
		rpc_server->reg(lock_protocol::convert, this, &HLockManager::Convert);
		rpc_server->reg(lock_protocol::subscribe, this, &HLockManager::Subscribe);
	}
}


HLockManager::~HLockManager()
{
	delete lm_;
}


lock_protocol::status
HLockManager::Acquire(int clt, int seq, lock_protocol::LockId lid, 
                      int mode_set, int flags, 
                      unsigned long long arg, int& mode_granted)
{
	return lm_->Acquire(clt, seq, lid, mode_set, flags, arg, mode_granted);
}


lock_protocol::status
HLockManager::AcquireVector(int clt, int seq, std::vector<lock_protocol::LockId> lidv, 
                           std::vector<int> modeiv, int flags, 
                           std::vector<unsigned long long> argv, int& num_locks_granted)
{
	return lm_->AcquireVector(clt, seq, lidv, modeiv, flags, argv, num_locks_granted);
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


lock_protocol::status
HLockManager::Subscribe(int clt, std::string id, int& unused)
{
	return lm_->Subscribe(clt, id, unused);
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
