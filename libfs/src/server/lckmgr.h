#ifndef _SERVER_LOCK_MANAGER_H_SAH189
#define _SERVER_LOCK_MANAGER_H_SAH189

#include "server/lckmgr.h"
#include <string>
#include <deque>
#include <set>
#include "rpc/rpc.h"
#include "server/lock_protocol.h"

namespace server {

class ClientRecord {
public:
	ClientRecord();
	ClientRecord(int, int);

	int clt_;
	int seq_;
};


struct Lock {
	Lock();
	~Lock();

	ClientRecord             owner_;
	int                      expected_clt_;
	std::deque<ClientRecord> waiting_list_;
	bool                     retry_responded_;
	bool                     revoke_sent_;
	pthread_cond_t           retry_responded_cv_;
};


class LockManager {
public:
	LockManager();
	~LockManager();
	lock_protocol::status stat(lock_protocol::LockId, int &);
	lock_protocol::status acquire(int, int, lock_protocol::LockId, int &);
	lock_protocol::status release(int, int, lock_protocol::LockId, int &);
	// subscribe for future notifications by telling the server the RPC addr
	lock_protocol::status subscribe(int, std::string, int &);
	void revoker();
	void retryer();
	void wait_acquie(lock_protocol::LockId);

private:
	std::map<int, rpcc *>                   clients_;
	std::map<lock_protocol::LockId, Lock>   locks_;
	std::set<lock_protocol::LockId>         revoke_set_;

	pthread_mutex_t                         mutex_;
	pthread_cond_t                          release_cv_;
	pthread_cond_t                          revoke_cv_;
	std::deque<lock_protocol::LockId>       released_locks_;
};


} // namespace server

#endif // _SERVER_LOCK_MANAGER_H_SAH189
