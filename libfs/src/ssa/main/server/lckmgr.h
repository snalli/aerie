#ifndef __STAMNOS_SSA_SERVER_LOCK_MANAGER_H
#define __STAMNOS_SSA_SERVER_LOCK_MANAGER_H

#include <string>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include <google/dense_hash_set>
#include <deque>
#include <set>
#include <vector>
#include "bcs/bcs.h"
#include "ssa/main/common/gtque.h"
#include "ssa/main/common/lock_protocol.h"

namespace ssa {
namespace cc {
namespace server {


class ClientRecord {
public:
	typedef int                 id_t;
	typedef lock_protocol::Mode Mode;

	ClientRecord();
	ClientRecord(id_t id, int seq, Mode mode);

	id_t id() const { return clt_; };
	Mode mode() const { return mode_; };
	void set_mode(Mode mode) { mode_ = mode; };

	int   seq_;
private:
	id_t  clt_;
	Mode  mode_;
};


struct Lock {
	enum Revoke {
		RVK_NO = lock_protocol::RVK_NO,      // no revoke
		RVK_NL = lock_protocol::RVK_NL,      
		RVK_XL2SL = lock_protocol::RVK_XL2SL,
		RVK_SR2SL = lock_protocol::RVK_SR2SL,
		RVK_XR2IX = lock_protocol::RVK_XR2IX,
		RVK_XR2XL = lock_protocol::RVK_XR2XL,
		RVK_XR2IXSL = lock_protocol::RVK_XR2IXSL,
		RVK_IXSL2IX = lock_protocol::RVK_IXSL2IX,
	};

	enum RevokeType {
		REVOKE_RELEASE=0, 
		REVOKE_DOWNGRADE, 
	};


	Lock(lock_protocol::LockId lid);
	~Lock();

	bool IsModeCompatibleInternal(int, int);
	bool IsModeCompatible(int);
	bool IsModeCompatibleExcludeClient(int, int);
	void PrintHolders(std::ostream);
	void AddHolderAndUpdateStatus(const ClientRecord&);
	void RemoveHolderAndUpdateStatus(int);
	void ConvertHolderAndUpdateStatus(int, int);

	GrantQueue<ClientRecord>     gtque_;
	int                          expected_clt_;
	std::deque<ClientRecord>     waiting_list_;
	bool                         retry_responded_;
	bool                         revoke_sent_;
	pthread_cond_t               retry_responded_cv_;
	lock_protocol::LockId        lid_;
};


class LockManager {
	typedef google::dense_hash_map<lock_protocol::LockId, Lock*> LockMap;
public:
	LockManager(::server::Ipc* ipc, bool register_handler, pthread_mutex_t* mutex = NULL);
	~LockManager();
	int Init(bool register_handler);
	Lock* FindLock(lock_protocol::LockId lid);
	Lock* FindOrCreateLock(lock_protocol::LockId lid);
	lock_protocol::status Stat(lock_protocol::LockId, int&);
	lock_protocol::status Acquire(int clt, int seq, lock_protocol::LockId lid, int mode_set, int flags, unsigned long long arg, int& mode_granted);
	lock_protocol::status Convert(int clt, int seq, lock_protocol::LockId lid, int mode, int flags, int& unused);
	lock_protocol::status Release(int clt, int seq, lock_protocol::LockId lid, int flags, int& unused);
	lock_protocol::Mode  SelectMode(Lock* lock, lock_protocol::Mode::Set mode_set);

	void revoker();
	void retryer();

	Lock* FindLockInternal(lock_protocol::LockId lid);
	Lock* FindOrCreateLockInternal(lock_protocol::LockId lid);
	lock_protocol::status AcquireInternal(int clt, int seq, Lock* l, lock_protocol::Mode::Set mode_set, int flags, int& mode_granted);
	lock_protocol::status ConvertInternal(int clt, int seq, Lock* l, lock_protocol::Mode mode, int flags, int& unused);

	lock_protocol::Mode LockMode(int clt, lock_protocol::LockId lid) 
	{
		Lock* l;
		if (locks_.find(lid) != locks_.end() && (l = locks_[lid])->gtque_.Exists(clt)) {
			printf("%p\n", l);
			ClientRecord* cr = l->gtque_.Find(clt);
			return cr->mode();
		}
		return lock_protocol::Mode::NONE;
	}

private:
	LockMap                                 locks_;
	std::set<lock_protocol::LockId>         revoke_set_;

	pthread_mutex_t*                        mutex_;
	pthread_cond_t                          available_cv_;
	pthread_cond_t                          revoke_cv_;
	/// Contains any locks that become available after a release or have being
	/// acquired in shared mode (and thus waiting clients can grab them)
	std::deque<lock_protocol::LockId>       available_locks_; 
	::server::Ipc*                          ipc_;
};


} // namespace server
} // namespace cc
} // namespace ssa

#endif // __STAMNOS_SSA_SERVER_LOCK_MANAGER_H
