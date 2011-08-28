/// \file lckmgr.h 
///
/// \brief Client lock manager interface.

#ifndef _CLIENT_LOCK_MANAGER_H_AFG819
#define _CLIENT_LOCK_MANAGER_H_AFG819

#include "lckmgr.h"
#include <string>
#include "rpc/rpc.h"
#include "server/lock_protocol.h"

namespace client {

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 6.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};



// SUGGESTED LOCK CACHING IMPLEMENTATION PLAN:
//
// to work correctly for lab 7,  all the requests on the server run till 
// completion and threads wait on condition variables on the client to
// wait for a lock.  this allows the server to be replicated using the
// replicated state machine approach.
//
// On the client a lock can be in several states:
//  - free: client owns the lock and no thread has it
//  - locked: client owns the lock and a thread has it
//  - acquiring: the client is acquiring ownership
//  - releasing: the client is releasing ownership
//
// in the state acquiring and locked there may be several threads
// waiting for the lock, but the first thread in the list interacts
// with the server and wakes up the threads when its done (released
// the lock).  a thread in the list is identified by its thread id
// (tid).
//
// a thread is in charge of getting a lock: if the server cannot grant
// it the lock, the thread will receive a retry reply.  at some point
// later, the server sends the thread a retry RPC, encouraging the client
// thread to ask for the lock again.
//
// once a thread has acquired a lock, its client obtains ownership of
// the lock. the client can grant the lock to other threads on the client 
// without interacting with the server. 
//
// the server must send the client a revoke request to get the lock back. this
// request tells the client to send the lock back to the
// server when the lock is released or right now if no thread on the
// client is holding the lock.  when receiving a revoke request, the
// client adds it to a list and wakes up a releaser thread, which returns
// the lock the server as soon it is free.
//
// the releasing is done in a separate a thread to avoid
// deadlocks and to ensure that revoke and retry RPCs from the server
// run to completion (i.e., the revoke RPC cannot do the release when
// the lock is free.
//
// a challenge in the implementation is that retry and revoke requests
// can be out of order with the acquire and release requests.  that
// is, a client may receive a revoke request before it has received
// the positive acknowledgement on its acquire request.  similarly, a
// client may receive a retry before it has received a response on its
// initial acquire request.  a flag field is used to record if a retry
// has been received.
//

class cached_lock {

public:
  enum lock_status {
    NONE, FREE, LOCKED, ACQUIRING,/* RELEASING (unused) */
  };

  // we use only a single cv to monitor changes of the lock's status
  // this may be less efficient because there would be many spurious
  // wake-ups, but it's simple anyway
  pthread_cond_t status_cv_;

  // condvar that is signaled when the ``used'' field is set to true
  pthread_cond_t used_cv_;

  pthread_cond_t got_acq_reply_cv_;
  
  // condvar that is signaled when the server informs the client to retry
  pthread_cond_t retry_cv_;

  // the sequence number of the latest *COMPLETED* acquire request made
  // by the client to obtain this lock
  // by completed, we mean the remote acquire() call returns with a value.
  pthread_t owner_;
  int seq_;
  bool used_; // set to true after first use
  int waiting_clients_; 
  bool can_retry_; // set when a retry message from the server is received

  cached_lock();
  ~cached_lock();

  // changes the status of this lock
  void set_status(lock_status);
  lock_status status() const;

private:
  lock_status status_;

};


class LockManager {
 private:
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;
  rpcs *rlsrpc_;
  rpcc *cl_;

  int last_seq;
  bool running;

  std::map<lock_protocol::lockid_t, cached_lock> cached_locks;

  // key: lock id; value: seq no. of the corresponding acquire
  std::map<lock_protocol::lockid_t, int> revoke_map;
  // global lock
  pthread_mutex_t m;
  // controls access to the revoke_map
  pthread_mutex_t revoke_m;
  pthread_cond_t revoke_cv;

  int do_acquire(lock_protocol::lockid_t);
  int do_release(lock_protocol::lockid_t);

 public:
  static int last_port;
  LockManager(std::string xdst, class lock_release_user *l = 0);
  ~LockManager();
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  virtual lock_protocol::status stat(lock_protocol::lockid_t);
  void releaser();

  rlock_protocol::status revoke(lock_protocol::lockid_t, int, int &);
  // tell this client to retry requesting the lock in which this client
  // was interest when that lock just became available
  rlock_protocol::status retry(lock_protocol::lockid_t, int, int &);
};


} // namespace client

#endif  // _CLIENT_LOCK_MANAGER_H_AFG819
