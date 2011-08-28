#ifndef _SERVER_LOCK_MANAGER_H_SAH189
#define _SERVER_LOCK_MANAGER_H_SAH189

#include "server/lckmgr.h"
#include <string>
#include <deque>
#include <set>
#include "rpc/rpc.h"
#include "server/lock_protocol.h"

namespace server {

class client_record {
 public:
  int clt;
  int seq;

  client_record();
  client_record(int, int);
};

struct lock_t {
  client_record owner;
  int expected_clt;
  std::deque<client_record> waiting_list;
  bool retry_responded;
  bool revoke_sent;
  pthread_cond_t retry_responded_cv;

  lock_t();
  ~lock_t();
};

class LockManager {

 private:
  std::map<int, rpcc *> clients;
  std::map<lock_protocol::lockid_t, lock_t> locks;
  std::set<lock_protocol::lockid_t> revoke_set;

  pthread_mutex_t m;
  pthread_cond_t release_cv_;
  pthread_cond_t revoke_cv_;
  std::deque<lock_protocol::lockid_t> released_locks;

 public:
  LockManager();
  ~LockManager();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  lock_protocol::status acquire(int, int, lock_protocol::lockid_t,
      int &);
  lock_protocol::status release(int, int, lock_protocol::lockid_t,
      int &);
  // subscribe for future notifications by telling the server the RPC addr
  lock_protocol::status subscribe(int, std::string, int &);
  void revoker();
  void retryer();
  void wait_acquie(lock_protocol::lockid_t);
};

} // namespace server

#endif // _SERVER_LOCK_MANAGER_H_SAH189
