// RPC stubs for clients to talk to lock_server, and cache the locks
// see lckmgr.h for protocol details.

#include "client/lckmgr.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "rpc/rpc.h"
#include "common/debug.h"

namespace client {

cached_lock::cached_lock()
  : owner_(0), seq_(0), used_(false), waiting_clients_(0), can_retry_(false),
    status_(NONE)
{
	pthread_cond_init(&status_cv_, NULL);
	pthread_cond_init(&used_cv_, NULL);
	pthread_cond_init(&retry_cv_, NULL);
	pthread_cond_init(&got_acq_reply_cv_, NULL);
}

cached_lock::~cached_lock()
{
	pthread_cond_destroy(&status_cv_);
	pthread_cond_destroy(&used_cv_);
	pthread_cond_destroy(&retry_cv_);
	pthread_cond_destroy(&got_acq_reply_cv_);
}

void
cached_lock::set_status(lock_status sts)
{
  // assume the thread holds the mutex m
  if (status_ != sts) {
    if (sts == LOCKED) {
      owner_ = pthread_self();
      used_ = true;
      pthread_cond_signal(&used_cv_);
    }
    if (sts == NONE) {
      // clear all fields
      used_ = false;
      can_retry_ = false;
    }
    status_ = sts;
    pthread_cond_broadcast(&status_cv_);
  }
}

cached_lock::lock_status
cached_lock::status() const
{
  return status_;
}

static void *
releasethread(void *x)
{
  LockManager *cc = (LockManager *) x;
  cc->releaser();
  return 0;
}

int LockManager::last_port = 0;

LockManager::LockManager(std::string xdst, 
				     class lock_release_user *_lu)
  : lu(_lu), last_seq(0)
{
  sockaddr_in dstsock;
  make_sockaddr(xdst.c_str(), &dstsock);
  cl_ = new rpcc(dstsock);
  if (cl_->bind() < 0) {
    printf("lock_client: call bind\n");
  }


  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // assert(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;

  pthread_mutex_init(&m, NULL);
  pthread_mutex_init(&revoke_m, NULL);
  pthread_cond_init(&revoke_cv, NULL);

  rlsrpc_ = new rpcs(rlock_port);
  /* register RPC handlers with rlsrpc_ */
  rlsrpc_->reg(rlock_protocol::revoke, this, &LockManager::revoke);
  rlsrpc_->reg(rlock_protocol::retry, this, &LockManager::retry);
  pthread_t th;
  int r = pthread_create(&th, NULL, &releasethread, (void *) this);
  assert (r == 0);
}

LockManager::~LockManager()
{
  pthread_mutex_lock(&m);
  std::map<lock_protocol::lockid_t, cached_lock>::iterator itr;
  for (itr = cached_locks.begin(); itr != cached_locks.end(); ++itr) {
    if (itr->second.status() == cached_lock::FREE) {
      do_release(itr->first);
    } else if (itr->second.status() == cached_lock::LOCKED
           && pthread_self() == itr->second.owner_) {
      release(itr->first);
      do_release(itr->first);
    }
    // TODO what about other states?
  }
  pthread_mutex_unlock(&m);
  pthread_cond_destroy(&revoke_cv);
  pthread_mutex_destroy(&m);
  delete rlsrpc_;
  delete cl_;
}

void
LockManager::releaser()
{

  // This method should be a continuous loop, waiting to be notified of
  // freed locks that have been revoked by the server, so that it can
  // send a release RPC.
  running = true;
  while (running) {
    pthread_mutex_lock(&m);
    while (revoke_map.empty()) {
      pthread_cond_wait(&revoke_cv, &m);
    }
    std::map<lock_protocol::lock_protocol::lockid_t, int>::iterator itr = revoke_map.begin();
    lock_protocol::lockid_t lid = itr->first;
    int seq = itr->second;
    dbg_log(DBG_INFO, "[%d] releasing lock %llu at seq %d\n", cl_->id(),
        lid, seq);
    cached_lock &l = cached_locks[lid];
    if (l.status() == cached_lock::NONE) {
      dbg_log(DBG_WARNING, "[%d] false revoke alarm: %llu\n", cl_->id(), lid);
      revoke_map.erase(lid);
      pthread_mutex_unlock(&m);
      continue;
    }
    while (l.seq_ < seq) {
      pthread_cond_wait(&l.got_acq_reply_cv_, &m);
    }
    while (!l.used_) {
      // wait until this lock is used at least once
      pthread_cond_wait(&l.used_cv_, &m);
    }
    while (l.status() != cached_lock::FREE) {
      // wait until the lock is released 
      pthread_cond_wait(&l.status_cv_, &m);
    }
    dbg_log(DBG_INFO, "[%d] calling release RPC for lock %llu\n", cl_->id(),
        lid);
    if (do_release(lid) == lock_protocol::OK) {
      // we set the lock's status to NONE instead of erasing it
      l.set_status(cached_lock::NONE);
      revoke_map.erase(lid);
    }
    // if remote release fails, we leave this lock in the revoke_map, which
    // will be released in a later attempt
    pthread_mutex_unlock(&m);
    usleep(500);
  }
}


// this function blocks until the specified lock is successfully acquired
// or if an unexpected error occurs.
// note that acquire() is NOT an atomic operation because it may temporarily
// release the mutex while waiting on certain condition varaibles.
// for this reason, we need an ACQUIRING status to tell other threads that
// an acquisition is in progress.
lock_protocol::status
LockManager::acquire(lock_protocol::lockid_t lid)
{
  lock_protocol::status r;

  if (last_seq == 0) {
    // this is my first contact with the server, so i have to tell him
    // my rpc address to subscribe for async rpc response
    int unused;
    if ((r = cl_->call(lock_protocol::subscribe, cl_->id(), id, unused)) !=
        lock_protocol::OK) {
      dbg_log(DBG_ERROR, "failed to subscribe client: %u\n", cl_->id());
      return r;
    }
  }

  pthread_mutex_lock(&m);
  cached_lock &l = cached_locks[lid];
  switch (l.status()) {
    case cached_lock::FREE:
      // great! no one is using the cached lock
      dbg_log(DBG_INFO, "[%d] lock %llu free locally: grant to %lu\n",
          cl_->id(), lid, (unsigned long)pthread_self());
      r = lock_protocol::OK;
      l.set_status(cached_lock::LOCKED);
      break;
    case cached_lock::ACQUIRING:
      // there is on-going lock acquisition; we just sit here and wait the
      // its completion, in which case we can safely fall through to the
      // next case
      dbg_log(DBG_INFO, "[%d] lck-%llu: another thread in acquisition\n",
          cl_->id(), lid);
      while (l.status() == cached_lock::ACQUIRING) {
        pthread_cond_wait(&l.status_cv_, &m);
      }
      if (l.status() == cached_lock::FREE) {
        // somehow this lock becomes mine!
        r = lock_protocol::OK;
        l.set_status(cached_lock::LOCKED);
        break;
      }
      // the lock is LOCKED or NONE, so we continue to the next case
    case cached_lock::LOCKED:
      if (l.owner_ == pthread_self()) {
        // the current thread has already obtained the lock
        dbg_log(DBG_INFO, "[%d] current thread already got lck %llu\n",
            cl_->id(), lid);
        r = lock_protocol::OK;
        break;
      } else {
        // in the predicate of the while loop, we don't check if the lock is
        // revoked by the server. this allows competition between the local
        // threads and the revoke thread.
        while (l.status() != cached_lock::FREE && l.status() !=
            cached_lock::NONE) {
          // TODO also check if there are many clients waiting
          pthread_cond_wait(&l.status_cv_, &m);
        }
        if (l.status() == cached_lock::FREE) {
          dbg_log(DBG_INFO, "[%d] lck %llu obatained locally by th %lu\n",
              cl_->id(), lid, (unsigned long)pthread_self());
          r = lock_protocol::OK;
          l.set_status(cached_lock::LOCKED);
          break;
        }
        // if we reach here, it means the lock has been returned to the
        // server, i.e., l.status() == cached_lock::NONE. we just fall through
      }
    case cached_lock::NONE:
      dbg_log(DBG_INFO, "[%d] lock %llu not available; acquiring now\n",
          cl_->id(), lid);
      l.set_status(cached_lock::ACQUIRING);
      while ((r = do_acquire(lid)) == lock_protocol::RETRY) {
        while (!l.can_retry_) {
          pthread_cond_wait(&l.retry_cv_, &m);
        }
      }
      if (r == lock_protocol::OK) {
        dbg_log(DBG_INFO, "[%d] thread %lu got lock %llu at seq %d\n",
            cl_->id(), pthread_self(), lid, l.seq_);
        l.set_status(cached_lock::LOCKED);
      }
      break;
    default:
      break;
  }
  pthread_mutex_unlock(&m);
  return r;
}

// release() is an atomic operation
lock_protocol::status
LockManager::release(lock_protocol::lockid_t lid)
{
  lock_protocol::status r = lock_protocol::OK;
  pthread_mutex_lock(&m);
  cached_lock &l = cached_locks[lid];
  if (l.status() == cached_lock::LOCKED && l.owner_ == pthread_self()) {
    assert(l.used_);
    if (l.waiting_clients_ >= 5) {
      // too many contending clients - we have to relinquish the lock
      // right now
      dbg_log(DBG_WARNING,
          "[%d] more than 5 clients waiting on lck %llu; release now\n",
          cl_->id(), lid);
      revoke_map.erase(lid);
      do_release(lid);
      // mark this lock as NONE anyway
      l.set_status(cached_lock::NONE);
    } else {
      l.set_status(cached_lock::FREE);
    }
  } else { 
    dbg_log(DBG_INFO, "[%d] thread %lu is not owner of lck %llu\n",
        cl_->id(), (unsigned long)pthread_self(), lid);
    r = lock_protocol::NOENT;
  }
  pthread_mutex_unlock(&m);
  return r;
}

rlock_protocol::status
LockManager::revoke(lock_protocol::lockid_t lid, int seq, int &unused)
{
  rlock_protocol::status r = rlock_protocol::OK;

  dbg_log(DBG_INFO,
      "[%d] server request to revoke lck %llu at seq %d\n", cl_->id(), lid,
      seq);
  // we do nothing but pushing back the lock id to the revoke queue
  pthread_mutex_lock(&revoke_m);
  revoke_map[lid] = seq;
  pthread_cond_signal(&revoke_cv);
  pthread_mutex_unlock(&revoke_m);
  return r;
}

rlock_protocol::status
LockManager::retry(lock_protocol::lockid_t lid, int seq,
    int &current_seq)
{
  rlock_protocol::status r = rlock_protocol::OK;
  pthread_mutex_lock(&m);
  assert(cached_locks.find(lid) != cached_locks.end());
  cached_lock &l = cached_locks[lid];
  if (seq >= l.seq_) {
    // it doesn't matter whether this retry message arrives before or
    // after the response to the corresponding acquire arrives, as long
    // as the sequence number of the retry matches that of the acquire
    assert(l.status() == cached_lock::ACQUIRING);
    dbg_log(DBG_INFO, "[%d] retry message for lid %llu seq %d\n",
        cl_->id(), lid, seq);
    l.can_retry_ = true;
    pthread_cond_signal(&l.retry_cv_);
  } else {
    dbg_log(DBG_WARNING,
        "[%d] outdated retry %d, current seq for lid %llu is %d\n", 
        cl_->id(), seq, lid, l.seq_);
  }

  pthread_mutex_unlock(&m);
  return r;
}

// assumes the current thread holds the mutex
int
LockManager::do_acquire(lock_protocol::lockid_t lid)
{
  int r, queue_len;
  cached_lock &l = cached_locks[lid];
  dbg_log(DBG_INFO, "[%d] calling acquire rpc for lck %llu id=%d seq=%d\n",
      cl_->id(), lid, cl_->id(), last_seq+1);
  r = cl_->call(lock_protocol::acquire, cl_->id(), ++last_seq, lid, queue_len);
  l.seq_ = last_seq;
  if (r == lock_protocol::OK) {
    l.waiting_clients_ = queue_len;
  } else if (r == lock_protocol::RETRY) {
    l.can_retry_ = false;
  }
  pthread_cond_signal(&l.got_acq_reply_cv_);
  return r;
}

int
LockManager::do_release(lock_protocol::lockid_t lid)
{
	int r;
	int unused;

	cached_lock &l = cached_locks[lid];
	if (lu) {
		lu->dorelease(lid);
	}
	r = cl_->call(lock_protocol::release, cl_->id(), l.seq_, lid, unused);
	return r;
}

int
LockManager::stat(lock_protocol::lockid_t lid)
{
	int r;
	int ret = cl_->call(lock_protocol::stat, cl_->id(), lid, r);
	
	assert (ret == lock_protocol::OK);
	return r;
}


} // namespace client
