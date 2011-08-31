// the caching lock server implementation

#include "server/lckmgr.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common/debug.h"

namespace server {

ClientRecord::ClientRecord()
	: clt_(-1), 
	  seq_(-1)
{

}


ClientRecord::ClientRecord(int clt, int seq)
	: clt_(clt), 
	  seq_(seq)
{

}


Lock::Lock()
	: expected_clt_(-1), 
	  retry_responded_(false), 
	  revoke_sent_(false)
{
	pthread_cond_init(&retry_responded_cv_, NULL);
}


Lock::~Lock()
{
	pthread_cond_destroy(&retry_responded_cv_);
}


static void *
revokethread(void *x)
{
	LockManager *lckmgr = (LockManager *) x;
	lckmgr->revoker();
	return 0;
}


static void *
retrythread(void *x)
{
	LockManager *lckmgr = (LockManager *) x;
	lckmgr->retryer();
	return 0;
}


LockManager::LockManager()
{
	pthread_mutex_init(&mutex_, NULL);
	pthread_cond_init(&revoke_cv_, NULL);
	pthread_cond_init(&release_cv_, NULL);

	pthread_t th;
	int r = pthread_create(&th, NULL, &revokethread, (void *) this);
	assert (r == 0);
	r = pthread_create(&th, NULL, &retrythread, (void *) this);
	assert (r == 0);
}


LockManager::~LockManager()
{
	pthread_mutex_lock(&mutex_);
	std::map<int, rpcc*>::iterator itr;
	for (itr = clients_.begin(); itr != clients_.end(); ++itr) {
		delete itr->second;
	}
	pthread_mutex_unlock(&mutex_);
	pthread_mutex_destroy(&mutex_);
	pthread_cond_destroy(&revoke_cv_);
	pthread_cond_destroy(&release_cv_);
}


lock_protocol::status
LockManager::acquire(int clt, int seq, lock_protocol::LockId lid,
    int &queue_len)
{
	lock_protocol::status r;
	dbg_log(DBG_INFO, "clt %d seq %d acquiring lock %llu\n", clt, seq, lid);
	pthread_mutex_lock(&mutex_);
	Lock &l = locks_[lid];
	queue_len = l.waiting_list_.size();
	dbg_log(DBG_INFO, "queue len for lock %llu: %d\n", lid, queue_len);
	if (l.owner_.clt_ == -1 && ((queue_len > 0 && l.expected_clt_ == clt) ||
	    queue_len == 0)) 
	{
		dbg_log(DBG_INFO, "lock %llu is free; granting to clt %d\n", lid, clt);
		l.owner_.clt_ = clt;
		l.owner_.seq_ = seq;
		r = lock_protocol::OK;
		if (queue_len != 0) {
			dbg_log(DBG_INFO, "expected clt %d replied to retry request\n", clt);
			l.expected_clt_ = -1;
			// since there are waiting clients_, we have to unfortunately add this
			// lock to the revoke set to get it back
			if (queue_len < 5) {
				revoke_set_.insert(lid);
				l.revoke_sent_ = true;
				pthread_cond_signal(&revoke_cv_);
			} else {
				// if the queue has more than 5 clients_ waiting, we don't need to
				// to send a revoke because we know the client will soon release
				// the lock. we just pretend that we have sent a revoke to the owner
				// of the lock
				l.revoke_sent_ = true;
			}
			//l.retry_responded_ = true;
			//pthread_cond_signal(&l.retry_responded_cv);
		} else {
			// a brand new lock
			l.revoke_sent_ = false;
		}
	} else {
		if (queue_len > 0) {
			// Note that we don't need to add lid to revoke_set_ here, because we
			// already did so for the head of the queue
			dbg_log(DBG_INFO, "clt %d not expected for lock %llu; queued\n",
			        clt, lid);
		} else {
			dbg_log(DBG_INFO, "queuing clt %d seq %d for lock %llu\n", 
			        clt, seq, lid);
			// i will be the head of the waiting list
			if (!l.revoke_sent_) {
				revoke_set_.insert(lid);
				l.revoke_sent_ = true;
				pthread_cond_signal(&revoke_cv_);
			}
		}
		l.waiting_list_.push_back(ClientRecord(clt, seq));
		r = lock_protocol::RETRY;
	}
	pthread_mutex_unlock(&mutex_);
	return r;
}

lock_protocol::status
LockManager::release(int clt, int seq, lock_protocol::LockId lid, int& unused)
{
	lock_protocol::status r = lock_protocol::OK;
	pthread_mutex_lock(&mutex_);
	if (locks_.find(lid) != locks_.end() && locks_[lid].owner_.clt_ == clt) {
		assert(locks_[lid].owner_.seq_ = seq);
		dbg_log(DBG_INFO, "clt %d released lck %llu at seq %d\n", clt, lid, seq);
		locks_[lid].owner_.clt_ = -1;
		locks_[lid].owner_.seq_ = -1;
		//locks_[lid].revoke_sent_ = false;
		released_locks_.push_back(lid);
		pthread_cond_signal(&release_cv_);
	}
	pthread_mutex_unlock(&mutex_);
	return r;
}

lock_protocol::status
LockManager::stat(lock_protocol::LockId lid, int &r)
{
	lock_protocol::status ret = lock_protocol::OK;
	r = 0;
	return ret;
}

lock_protocol::status
LockManager::subscribe(int clt, std::string id, int &unused)
{
	sockaddr_in           dstsock;
	rpcc*                 cl;
	lock_protocol::status r = lock_protocol::OK;

	pthread_mutex_lock(&mutex_);
	make_sockaddr(id.c_str(), &dstsock);
	cl = new rpcc(dstsock);
	if (cl->bind() == 0) {
		clients_[clt] = cl;
	} else {
		printf("failed to bind to clt %d\n", clt);
	}
	pthread_mutex_unlock(&mutex_);
	return r;
}



/// This method is a continuous loop, that sends revoke messages to
/// lock holders whenever another client wants the same lock.
void
LockManager::revoker()
{
	while (true) {
		pthread_mutex_lock(&mutex_);
		while (revoke_set_.empty()) {
			pthread_cond_wait(&revoke_cv_, &mutex_);
		}
		std::set<lock_protocol::LockId>::iterator itr = revoke_set_.begin();
		lock_protocol::LockId lid = *itr;
		revoke_set_.erase(lid);
		int unused;
		Lock &l = locks_[lid];
		rpcc *cl = clients_[l.owner_.clt_];
		if (cl) {
			if (cl->call(rlock_protocol::revoke, lid, l.owner_.seq_, unused)
			    != rlock_protocol::OK) 
			{
				dbg_log(DBG_ERROR, "failed to send revoke\n");
			}
		} else {
			dbg_log(DBG_ERROR, "client %d didn't subscribe\n", l.owner_.clt_);
		}
		pthread_mutex_unlock(&mutex_);
		usleep(500);
	}
}


/// This method is a continuous loop, waiting for locks to be released
/// and then sending retry messages to those who are waiting for it.
void
LockManager::retryer()
{

	while (true) {
		pthread_mutex_lock(&mutex_);
		while (released_locks_.empty()) {
			pthread_cond_wait(&release_cv_, &mutex_);
		}
		lock_protocol::LockId lid = released_locks_.front();
		// XXX warning: this is not fault-tolerant
		released_locks_.pop_front();
		Lock &l = locks_[lid];
		std::deque<ClientRecord> &wq = l.waiting_list_;
		ClientRecord *cr = NULL;
		if (!wq.empty()) {
			cr = &wq.front();
			l.expected_clt_ = cr->clt_;
			wq.pop_front();
		}
		pthread_mutex_unlock(&mutex_);

		if (cr) {
			int cur_seq;
			// TODO place a time limit on the retry for this client
			if (clients_[cr->clt_]->call(rlock_protocol::retry, lid, cr->seq_,
				cur_seq) == rlock_protocol::OK) 
			{
				dbg_log(DBG_INFO,
				        "successfully sent a retry to clt %d seq %d for lck %llu\n",
				        cr->clt_, cr->seq_, lid); 
			} else {
				dbg_log(DBG_ERROR,
				        "failed to tell client %d to retry lock %llu\n", 
				        cr->clt_, lid);
			}
		}
    //usleep(500);
	}
}

} // namespace server
