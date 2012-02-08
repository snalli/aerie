/// \file hlckmgr.h
///
/// \brief Server hierarchical lock manager

#ifndef __STAMNOS_DPO_SERVER_HIERARCHICAL_LOCK_MANAGER_H
#define __STAMNOS_DPO_SERVER_HIERARCHICAL_LOCK_MANAGER_H

#include <pthread.h>
#include "dpo/base/server/lckmgr.h"
#include "rpc/rpc.h"


namespace dpo {
namespace cc {
namespace server {


class HLockManager {
public:
	HLockManager(rpcs* rpc_server);
	~HLockManager();

	int Init();

	lock_protocol::status Stat(lock_protocol::LockId, int&);
	lock_protocol::status Acquire(int clt, int seq, unsigned long long lid_u64, int mode_set, int flags, unsigned long long arg, int& mode_granted);
	lock_protocol::status Convert(int clt, int seq, unsigned long long lid_u64, int mode, int flags, int& unused);
	lock_protocol::status Release(int clt, int seq, unsigned long long lid_u64, int flags, int& unused);
	lock_protocol::status Subscribe(int, std::string, int&);

private:
	rpcs*           rpc_server_;
	LockManager*    lm_;
	pthread_mutex_t mutex_;
};


} // namespace server
} // namespace cc
} // namespace dpo

#endif // __STAMNOS_DPO_SERVER_HIERARCHICAL_LOCK_MANAGER_H
