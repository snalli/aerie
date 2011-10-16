/// \file hlckmgr.h
///
/// \brief Server hierarchical lock manager

#ifndef _SERVER_HIERARCHICAL_LOCK_MANAGER_H_APL189
#define _SERVER_HIERARCHICAL_LOCK_MANAGER_H_APL189

#include "server/lckmgr.h"
#include "rpc/rpc.h"

namespace server {


class HLockManager {
public:
	HLockManager(rpcs* rpc_server);
	~HLockManager();
	lock_protocol::status Stat(lock_protocol::LockId, int&);
	lock_protocol::status Acquire(int clt, int seq, lock_protocol::LockId lid, int mode_set, int flags, unsigned long long arg, int& mode_granted);
	lock_protocol::status AcquireVector(int clt, int seq, std::vector<lock_protocol::LockId> lidv, std::vector<int> modeiv, int flags, std::vector<unsigned long long> argv, int& num_locks_granted);
	lock_protocol::status Convert(int clt, int seq, lock_protocol::LockId lid, int mode, int flags, int& unused);
	lock_protocol::status Release(int clt, int seq, lock_protocol::LockId lid, int flags, int& unused);
	lock_protocol::status Subscribe(int, std::string, int&);

private:
	LockManager* lm_;

};


} // namespace server


#endif  /* _SERVER_HIERARCHICAL_LOCK_MANAGER_H_APL189 */
