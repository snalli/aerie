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
	  seq_(-1),
	  req_lck_mode_(Lock::NONE)

{

}


ClientRecord::ClientRecord(int clt, int seq, int req_lck_mode)
	: clt_(clt), 
	  seq_(seq),
	  req_lck_mode_(req_lck_mode)
{

}


Lock::Lock()
	: status_(Lock::FREE),
	  expected_clt_(-1), 
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
	pthread_cond_init(&available_cv_, NULL);

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
	pthread_cond_destroy(&available_cv_);
}


lock_protocol::status
LockManager::acquire(int clt, int seq, lock_protocol::LockId lid, int req_lck_mode,
                     int &queue_len)
{
	lock_protocol::status r;
	dbg_log(DBG_INFO, "clt %d seq %d acquiring lock %llu (%s)\n", clt, seq, 
	        lid, req_lck_mode == Lock::EXCLUSIVE ? "EXCLUSIVE": "SHARED");
	pthread_mutex_lock(&mutex_);
	Lock& l = locks_[lid];
	queue_len = l.waiting_list_.size();
	dbg_log(DBG_INFO, "queue len for lock %llu: %d\n", lid, queue_len);

	if (((req_lck_mode == Lock::EXCLUSIVE && l.status_ == Lock::FREE) && 
	     (queue_len == 0 || (queue_len > 0 && l.expected_clt_ == clt)))	||
	    ((req_lck_mode == Lock::SHARED && 
		  (l.status_ == Lock::FREE || l.status_ == Lock::SHARED)) && 
	     (queue_len == 0 || (queue_len > 0 && l.expected_clt_ == clt)))
	   )
	{
		dbg_log(DBG_INFO, "lock %llu is %s; granting to clt %d\n", lid, 
		        l.status_ == Lock::FREE ? "free": "shared", clt);
		assert(l.holders_.find(clt) == l.holders_.end());
		l.holders_[clt] = ClientRecord(clt, seq, req_lck_mode);
		if (req_lck_mode == Lock::EXCLUSIVE) {
			l.status_ = Lock::EXCLUSIVE;
		} else {
			l.status_ = Lock::SHARED;
		}
		r = lock_protocol::OK;
		if (queue_len != 0) {
			dbg_log(DBG_INFO, "expected clt %d replied to retry request\n", clt);
			l.expected_clt_ = -1;
			// Since there are clients waiting, we have to unfortunately add this
			// lock to the revoke set to get it back.
			// Check the type of request (S or X) of the next client in line to 
			// decide the type of revocation.
			revoke_set_.insert(lid);
			std::deque<ClientRecord> &wq = l.waiting_list_;
			ClientRecord* cr = &wq.front();
			if (cr->req_lck_mode_ == Lock::EXCLUSIVE) { 
				l.revoke_type_ = Lock::REVOKE_RELEASE;
			} else {
				if (req_lck_mode == Lock::SHARED) {
					// Client is acquiring the lock in SHARED mode so don't send 
					// him a revoke msg as the next client in line is waiting to
					// acquire the lock in SHARED mode too. Send the waiting 
					// client a retry.
					available_locks_.push_back(lid);
					pthread_cond_signal(&available_cv_);
				} else {
					l.revoke_type_ = Lock::REVOKE_DOWNGRADE;
				}
			}
			l.revoke_sent_ = true;
			pthread_cond_signal(&revoke_cv_);
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
				if (req_lck_mode == Lock::SHARED) {
					l.revoke_type_ = Lock::REVOKE_DOWNGRADE;
				} else {
					l.revoke_type_ = Lock::REVOKE_RELEASE;
				}
				l.revoke_sent_ = true;
				pthread_cond_signal(&revoke_cv_);
			}
		}
		l.waiting_list_.push_back(ClientRecord(clt, seq, req_lck_mode));
		r = lock_protocol::RETRY;
	}
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status
LockManager::acquire_exclusive(int clt, int seq, lock_protocol::LockId lid, 
                               int& queue_len)
{
	return acquire(clt, seq, lid, Lock::EXCLUSIVE, queue_len);
}


lock_protocol::status
LockManager::acquire_shared(int clt, int seq, lock_protocol::LockId lid, 
                            int& queue_len)
{
	return acquire(clt, seq, lid, Lock::SHARED, queue_len);
}


lock_protocol::status
LockManager::release(int clt, int seq, lock_protocol::LockId lid, int& unused)
{
	std::map<int, ClientRecord>::iterator  itr_icr;
	lock_protocol::status                  r = lock_protocol::OK;
	pthread_mutex_lock(&mutex_);

	if (locks_.find(lid) != locks_.end() && 
	    locks_[lid].holders_.find(clt) != locks_[lid].holders_.end())
	{
		assert(locks_[lid].holders_[clt].seq_ == seq);
		dbg_log(DBG_INFO, "clt %d released lck %llu at seq %d\n", clt, lid, seq);
		//TODO: remove holder and if no more holders set lock free
		locks_[lid].holders_.erase(clt);
		if (locks_[lid].holders_.empty()) {
			locks_[lid].status_ = Lock::FREE;
			//locks_[lid].revoke_sent_ = false;
			available_locks_.push_back(lid);
			pthread_cond_signal(&available_cv_);
		}
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
	std::set<lock_protocol::LockId>::iterator itr_l;
	std::map<int, ClientRecord>::iterator     itr_icr;
	lock_protocol::LockId                     lid;
	int                                       rpc_method;
	int                                       clt;

	while (true) {
		pthread_mutex_lock(&mutex_);
		while (revoke_set_.empty()) {
			pthread_cond_wait(&revoke_cv_, &mutex_);
		}
		itr_l = revoke_set_.begin();
		lid = *itr_l;
		revoke_set_.erase(lid);
		Lock& l = locks_[lid];
		if (l.revoke_type_ == Lock::REVOKE_DOWNGRADE) {
			rpc_method = rlock_protocol::revoke_downgrade;
		} else {
			rpc_method = rlock_protocol::revoke_release;
		}
		for (itr_icr = l.holders_.begin(); 
		     itr_icr != l.holders_.end(); itr_icr++) 
		{
			int           unused;
			int           clt = (*itr_icr).first;
			ClientRecord& cr = (*itr_icr).second;
			rpcc*         cl = clients_[clt];
			if (cl) {
				if (cl->call(rpc_method, lid, cr.seq_, unused)
					!= rlock_protocol::OK) 
				{
					dbg_log(DBG_ERROR, "failed to send revoke\n");
				}
			} else {
				dbg_log(DBG_ERROR, "client %d didn't subscribe\n", clt);
			}
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
		while (available_locks_.empty()) {
			pthread_cond_wait(&available_cv_, &mutex_);
		}
		lock_protocol::LockId lid = available_locks_.front();
		// XXX warning: this is not fault-tolerant
		available_locks_.pop_front();
		Lock &l = locks_[lid];
		std::deque<ClientRecord>& wq = l.waiting_list_;
		ClientRecord* cr = NULL;
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
				//FIXME: if cannot reach client then we need to send a retry to the
				//next waiting client. But here you don't actually know whether client
				//got the call but just didn't reply. so you need to do some heart beat
				//detection to figure whether the client is dead. If finally you decide
				//to proceed with the next waiting client we need to ensure that he 
				//requested the lock in the same mode. For example, if the lock is available
				//in SHARED mode, we reached this function because the now dead client
				//requested the lock in SHARED mode so the lock was available for him
				//to grab. However, if the next client requires the lock in EXCLUSIVE 
				//mode then sending him a retry is unnecessary as we know that he will
				//fail.
			}
		}
		//usleep(500);
	}
}

} // namespace server
