// the caching lock server implementation

#include "server/lckmgr.h"
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include "common/debug.h"
#include "common/gtque.h"
#include "common/lock_protocol.h"
#include "common/lock_protocol-static.h"

// TODO: leases
// Server does not trust clients so it should adopt timeouts to expire 
// lock acquisitions and expire retry requests that are not followed by
// acquisition.
// 1) lock acquisition expires after time Tacq after sending a revoke msg.
// 2) lock retry expires after time Tr after sending a retry msg. This allows
//    to provide some degree of fairness without letting a malicious client
//    starve other clients.
//
// Locks are ordered by time of expiration. A thread periodically checks
// if there are any expired locks and collects such locks. 



namespace server {

// we ask the client to revoke the lock to a mode which is at least compatible to
// the conflicting lock request.
static int revoke_table[8][8] = {
	/*                                                              Requested Lock Mode                                                           */
	/*            NL,           SL,                SR,           IS,              IX,              XL,                XR,           IXSL          */
	/* NL   */ {  Lock::RVK_NO, Lock::RVK_NO,      Lock::RVK_NO, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NO,      Lock::RVK_NO, Lock::RVK_NO },
	/* SL   */ {  Lock::RVK_NO, Lock::RVK_NO,      Lock::RVK_NO, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NL,      Lock::RVK_NL, Lock::RVK_NO },
	/* SR   */ {  Lock::RVK_NO, Lock::RVK_NO,      Lock::RVK_NO, Lock::RVK_NO,    Lock::RVK_SR2SL, Lock::RVK_NL,      Lock::RVK_NL, Lock::RVK_SR2SL },
	/* IS   */ {  Lock::RVK_NO, Lock::RVK_NO,      Lock::RVK_NO, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NO,      Lock::RVK_NL, Lock::RVK_NO },
	/* IX   */ {  Lock::RVK_NO, Lock::RVK_NO,      Lock::RVK_NL, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NO,      Lock::RVK_NL, Lock::RVK_NO },
	/* XL   */ {  Lock::RVK_NO, Lock::RVK_XL2SL,   Lock::RVK_NL, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NL,      Lock::RVK_NL, Lock::RVK_XL2SL },
	/* XR   */ {  Lock::RVK_NO, Lock::RVK_XR2IXSL, Lock::RVK_NL, Lock::RVK_XR2XL, Lock::RVK_XR2XL, Lock::RVK_XR2IX,   Lock::RVK_NL, Lock::RVK_XR2IXSL },
	/* IXSL */ {  Lock::RVK_NO, Lock::RVK_NO,      Lock::RVK_NL, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_IXSL2IX, Lock::RVK_NL, Lock::RVK_NO },
};



ClientRecord::ClientRecord()
	: clt_(-1), 
	  seq_(-1),
	  mode_(lock_protocol::Mode::NL)
{ }


ClientRecord::ClientRecord(ClientRecord::id_t clt, int seq, ClientRecord::Mode mode)
	: clt_(clt), 
	  seq_(seq),
	  mode_(mode)
{ }


Lock::Lock(lock_protocol::LockId lid)
	: lid_(lid),
	  expected_clt_(-1), 
	  retry_responded_(false), 
	  revoke_sent_(false),
	  gtque_(lock_protocol::Mode::CARDINALITY, lock_protocol::Mode(lock_protocol::Mode::NL))
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
	LockManager* lckmgr = (LockManager *) x;
	lckmgr->revoker();
	return 0;
}


static void *
retrythread(void *x)
{
	LockManager* lckmgr = (LockManager *) x;
	lckmgr->retryer();
	return 0;
}

// ISSUE: Bad software engineering: 
// We want to be able to use LockManager as a building block
// for other lock managers such as hierarchical lock manager.
// That's why we need to construct some of the components of
// the lock manager such as the global mutex.
// this however breaks encapsulation.
LockManager::LockManager(rpcs* rpc_server, pthread_mutex_t* mutex)
	: mutex_(mutex)
{
	assert(Init(rpc_server) == 0);
}


int
LockManager::Init(rpcs* rpc_server)
{
	pthread_t th;
	int       r;

	if (!mutex_) {
		mutex_ = new pthread_mutex_t;
		pthread_mutex_init(mutex_, NULL);
	}
	pthread_cond_init(&revoke_cv_, NULL);
	pthread_cond_init(&available_cv_, NULL);
	locks_.set_empty_key(-1);
	r = pthread_create(&th, NULL, &revokethread, (void *) this);
	assert (r == 0);
	r = pthread_create(&th, NULL, &retrythread, (void *) this);
	assert (r == 0);
	
	if (rpc_server) {
		rpc_server->reg(lock_protocol::stat, this, &LockManager::Stat);
		rpc_server->reg(lock_protocol::acquire, this, &LockManager::Acquire);
		rpc_server->reg(lock_protocol::acquirev, this, &LockManager::AcquireVector);
		rpc_server->reg(lock_protocol::release, this, &LockManager::Release);
		rpc_server->reg(lock_protocol::convert, this, &LockManager::Convert);
		rpc_server->reg(lock_protocol::subscribe, this, &LockManager::Subscribe);
	}
	return 0;
}


LockManager::~LockManager()
{
	std::map<int, rpcc*>::iterator itr;

	pthread_mutex_lock(mutex_);
	for (itr = clients_.begin(); itr != clients_.end(); ++itr) {
		delete itr->second;
	}
	pthread_mutex_unlock(mutex_);
	//pthread_mutex_destroy(mutex_);
	pthread_cond_destroy(&revoke_cv_);
	pthread_cond_destroy(&available_cv_);
}


/// \brief Returns the lock lid if it exists, otherwise it returns NULL
/// Assumes caller has the mutex LockManager::mutex_
inline Lock*
LockManager::FindLockInternal(lock_protocol::LockId lid)
{
	Lock* lp;

	// if lid does not exist then the initial value of the slot is 0 (NULL)
	lp = locks_[lid];
	return lp;
}


/// \brief Returns the lock lid. If the lock does not exist, it first creates 
/// the lock.
/// Assumes caller has the mutex LockManager::mutex_
inline Lock*
LockManager::FindOrCreateLockInternal(lock_protocol::LockId lid)
{
	Lock* lp;

	lp = locks_[lid];
	if (lp == NULL) {
		DBG_LOG(DBG_INFO, DBG_MODULE(client_lckmgr), 
		        "Creating lock %llu\n", lid);
		lp = new Lock(lid);
		locks_[lid] = lp;
	}	
	return lp;
}


/// \brief Finds the lock identified by lid. 
///
/// \param lid lock identifier. 
/// \return a reference (pointer) to the lock.
///
/// Does no reference counting.
Lock*
LockManager::FindLock(lock_protocol::LockId lid)
{
	Lock* l;

	pthread_mutex_lock(mutex_);
	l = FindLockInternal(lid);
	pthread_mutex_unlock(mutex_);
	return l;
}


/// \brief Finds the lock identified by lid. If the lock lid does not exist 
/// it first creates the lock.
///
/// \param lid lock identifier. 
/// \return a reference (pointer) to the lock.
///
/// Does no reference counting.
Lock*
LockManager::FindOrCreateLock(lock_protocol::LockId lid)
{
	Lock* l;

	pthread_mutex_lock(mutex_);
	l = FindOrCreateLockInternal(lid);
	pthread_mutex_unlock(mutex_);
	return l;
}


lock_protocol::Mode
LockManager::SelectMode(Lock* lock, lock_protocol::Mode::Set mode_set)
{
	/* Pick the most severe that can be granted instantly, or
	 * wait for the least severe */
	
	lock_protocol::Mode mode = mode_set.MostSevere(lock_protocol::Mode::NL);
	if (!lock->gtque_.CanGrant(mode)) {
		mode = mode_set.LeastSevere();
	}
	dbg_log(DBG_INFO, "selecting mode %s out of modes {%s}\n",  
	        mode.String().c_str(), mode_set.String().c_str());
	return mode;
}

/// assumption: caller has mutex_
lock_protocol::status
LockManager::AcquireInternal(int clt, int seq, Lock* l, 
                             lock_protocol::Mode::Set mode_set, int flags, 
                             int& mode_granted)
{
	char                  statestr[128];
	uint32_t              next_state;
	int                   wq_len;
	lock_protocol::status r;
	lock_protocol::Mode   mode;
	
	dbg_log(DBG_INFO, "clt %d seq %d acquiring lock %llu (%s)\n", clt, seq, 
	        l->lid_, mode_set.String().c_str());
	mode = SelectMode(l, mode_set);
	wq_len = l->waiting_list_.size();
	dbg_log(DBG_INFO, "queue len for lock %llu: %d\n", l->lid_, wq_len);
	if ((wq_len == 0 || (wq_len > 0 && l->expected_clt_ == clt)) &&
		l->gtque_.CanGrant(mode)) 
	{
		dbg_log(DBG_INFO, "lock %llu is compatible; granting to clt %d\n", 
		        l->lid_, clt);
		r = lock_protocol::OK;
		l->gtque_.Add(ClientRecord(clt, seq, mode));
		l->expected_clt_ = -1;
		if (wq_len != 0) {
			// Since there are clients waiting, we have two options. If the 
			// request of the next client in line is:
			// 1) compatible with the just serviced request, we send a retry 
			//    msg to the waiting client.
			// 2) incompatible with the just serviced request, we add the lock
			//    in the revoke set to get it back.
			std::deque<ClientRecord>& wq = l->waiting_list_;
			ClientRecord&             cr = wq.front();
			if (l->gtque_.CanGrant(cr.mode())) {
				available_locks_.push_back(l->lid_);
				pthread_cond_signal(&available_cv_);
			} else {
				revoke_set_.insert(l->lid_);
				l->revoke_sent_ = true;
				pthread_cond_signal(&revoke_cv_);
			}
		} else {
			// a brand new lock
			l->revoke_sent_ = false;
		}
		mode_granted = mode.value();
	} else {
		if ((flags & lock_protocol::FLG_NOQUE) == 0) {
			if (wq_len > 0) {
				// Note that we don't need to add lid to revoke_set_ here, because we
				// already did so for the head of the queue
				dbg_log(DBG_INFO, "clt %d not expected for lock %llu; queued\n",
						clt, l->lid_);
			} else {
				dbg_log(DBG_INFO, "queuing clt %d seq %d for lock %llu\n", 
						clt, seq, l->lid_);
				// i will be the head of the waiting list
				if (!l->revoke_sent_) {
					revoke_set_.insert(l->lid_);
					l->revoke_sent_ = true;
					pthread_cond_signal(&revoke_cv_);
				}
			}
			l->waiting_list_.push_back(ClientRecord(clt, seq, mode));
		}
		r = lock_protocol::RETRY;
	}
	return r;
}


lock_protocol::status
LockManager::Acquire(int clt, int seq, lock_protocol::LockId lid, 
                     int mode_set, int flags, 
                     unsigned long long arg, int& mode_granted)
{
	lock_protocol::status r;
	Lock*                 lock;

	pthread_mutex_lock(mutex_);
	lock = FindOrCreateLockInternal(lid);
	r = AcquireInternal(clt, seq, lock, lock_protocol::Mode::Set(mode_set), 
	                    flags, mode_granted);
	pthread_mutex_unlock(mutex_);
	return r;
}


lock_protocol::status
LockManager::AcquireVector(int clt, int seq, std::vector<lock_protocol::LockId> lidv, 
                           std::vector<int> modeiv, int flags, 
                           std::vector<unsigned long long> argv, int& num_locks_granted)
{
	lock_protocol::status                         r;
	std::vector<lock_protocol::LockId>::iterator  lidv_itr;
	std::vector<int>::iterator                    modeiv_itr;
	
	for (lidv_itr = lidv.begin(), modeiv_itr = modeiv.begin(); 
		lidv_itr != lidv.end() && modeiv_itr != modeiv.end(); 
		lidv_itr++, modeiv_itr++) 
	{
		printf("lid=%llu, mode=%s\n", *lidv_itr, lock_protocol::Mode(static_cast<lock_protocol::Mode::Enum>(*modeiv_itr)).String().c_str());
	}

	//TODO

	r = lock_protocol::OK;

	return r;
}


// convert does not block to avoid any deadlocks.
lock_protocol::status
LockManager::ConvertInternal(int clt, int seq, Lock* l, 
                             lock_protocol::Mode new_mode, 
                             int flags, int& unused)
{
	std::map<int, ClientRecord>::iterator itr_icr;
	lock_protocol::status                 r = lock_protocol::NOENT;
	int                                   next_status;

	if (locks_.find(l->lid_) != locks_.end() && locks_[l->lid_]->gtque_.Exists(clt))
	{
		ClientRecord* cr = l->gtque_.Find(clt);
		dbg_log(DBG_INFO, "clt %d convert lck %llu at seq %d (%s --> %s)\n", 
		        clt, l->lid_, seq, cr->mode().String().c_str(), 
				new_mode.String().c_str());
		assert(cr->seq_ == seq);
		// if there is an outstanding revoke request then 
		// the simplest approach is to deny the conversion
		// as the new mode could conflict with the new mode
		// requested by revoke.
		// We only grant the conversion if it does not 
		// upgrade the current mode.
		if (l->revoke_sent_ &&
		    !l->gtque_.IsModeSet(new_mode) &&
			l->gtque_.PartialOrder(new_mode) >= 0)
		{
			r = lock_protocol::RETRY;
			goto out;
		}
		// if requested mode cannot be granted immediately then don't
		// block-wait to avoid deadlock
		if (l->gtque_.ConvertInPlace(clt, new_mode) < 0) {
			r = lock_protocol::RETRY;
			goto out;
		}
		// Check whether the lock can be made available to the next waiting client.
		// But first ensure there is no outstanding acquire which has been woken
		// up to acquire the lock (expected_clt == -1), otherwise we could end  
		// up making the lock available to a second client. 
		if (l->expected_clt_ == -1) {
			std::deque<ClientRecord>& wq = l->waiting_list_;
			if (!wq.empty()) {
				ClientRecord& wcr = wq.front();
				if (l->gtque_.CanGrant(wcr.mode())) {
					available_locks_.push_back(l->lid_);
					pthread_cond_signal(&available_cv_);
				}
			}
		}
		r = lock_protocol::OK;
	}

out:
	return r;
}


// convert does not block to avoid any deadlocks.
lock_protocol::status
LockManager::Convert(int clt, int seq, lock_protocol::LockId lid, 
                     int new_mode, int flags, int& unused)
{
	lock_protocol::status r;
	Lock*                 lock;

	pthread_mutex_lock(mutex_);
	lock = FindOrCreateLockInternal(lid);
	lock_protocol::Mode::Enum enum_new_mode = static_cast<lock_protocol::Mode::Enum>(new_mode);
	r = ConvertInternal(clt, seq, lock, lock_protocol::Mode(enum_new_mode), flags, unused);
	pthread_mutex_unlock(mutex_);
	return r;
}


lock_protocol::status
LockManager::Release(int clt, int seq, lock_protocol::LockId lid, int flags, int& unused)
{
	lock_protocol::status r;
	Lock*                 lock;

	dbg_log(DBG_INFO, "clt %d release lck %llu at seq %d\n", 
	        clt, lid, seq);

	pthread_mutex_lock(mutex_);
	lock = FindOrCreateLockInternal(lid);
	r = ConvertInternal(clt, seq, lock, lock_protocol::Mode(lock_protocol::Mode::NL), flags, unused);
	pthread_mutex_unlock(mutex_);
	return r;
}


lock_protocol::status
LockManager::Stat(lock_protocol::LockId lid, int& r)
{
	lock_protocol::status ret = lock_protocol::OK;
	r = 0;
	return ret;
}


lock_protocol::status
LockManager::Subscribe(int clt, std::string id, int& unused)
{
	sockaddr_in           dstsock;
	rpcc*                 cl;
	lock_protocol::status r = lock_protocol::OK;

	pthread_mutex_lock(mutex_);
	make_sockaddr(id.c_str(), &dstsock);
	cl = new rpcc(dstsock);
	if (cl->bind() == 0) {
		clients_[clt] = cl;
	} else {
		printf("failed to bind to clt %d\n", clt);
	}
	pthread_mutex_unlock(mutex_);
	return r;
}


struct RevokeMsg {
	RevokeMsg(int clt, int seq, int revoke_type)
		: clt_(clt),
		  seq_(seq),
		  revoke_type_(revoke_type)
	{ }
	
	int clt_;
	int seq_;
	int revoke_type_;
};


/// This method is a continuous loop, that sends revoke messages to
/// lock holders whenever another client wants the same lock.
void
LockManager::revoker()
{
	std::set<lock_protocol::LockId>::iterator itr_l;
	GrantQueue<ClientRecord>::iterator        itr_icr;
	lock_protocol::LockId                     lid;
	int                                       clt;
	int                                       revoke_type;
	std::vector<struct RevokeMsg>             revoke_msgs;
	std::vector<struct RevokeMsg>::iterator   itr_r;

	while (true) {
		pthread_mutex_lock(mutex_);
		while (revoke_set_.empty()) {
			pthread_cond_wait(&revoke_cv_, mutex_);
		}
		itr_l = revoke_set_.begin();
		lid = *itr_l;
		revoke_set_.erase(lid);
		Lock* l = locks_[lid];
		ClientRecord& waiting_cr = l->waiting_list_.front();

		// ISSUE: distributed deadlock
		// doing RPC to the client while holding the global mutex may cause a 
		// distributed deadlock if we block behind a client who is blocked 
		// behind us (because its RPC request is blocked behind the mutex)
		// in general we cannot trust the client so we should never block
		// behind a client (use timeout to expire RPC request). 
		// here we drop the mutex so that we can service other incoming RPC 
		// requests. 
		// Dropping the mutex is safe because the lock server imposes the 
		// invariant that a revoke request is send when a competing client
		// needs the lock, and all other requests for the particular lock 
		// are queued behind that clients request. so we know that no other 
		// clients will be added in the gtque that need to be dropped.
		// make a local copy of the requests to be send and then release 
		// the mutex.
		for (itr_icr = l->gtque_.begin(); 
		     itr_icr != l->gtque_.end(); itr_icr++) 
		{
			int           clt = (*itr_icr).first;
			ClientRecord& cr = (*itr_icr).second;
			rpcc*         cl = clients_[clt];
			revoke_type = revoke_table[cr.mode().value()][waiting_cr.mode().value()];
			if (revoke_type == Lock::RVK_NO) {
				// false alarm 
				// this could happen if the server converts the lock to a 
				// compatible mode before sending the revoke request to 
				// the client (because the client volunteerily converted 
				// the lock) 
				continue;
			}
			revoke_msgs.push_back(RevokeMsg(clt, cr.seq_, revoke_type));
		}
		pthread_mutex_unlock(mutex_);

		for (itr_r = revoke_msgs.begin(); itr_r != revoke_msgs.end(); itr_r++) {
			int           unused;
			dbg_log(DBG_INFO, "revoke client %d lock %llu: \n", clt, lid, revoke_type);

			rpcc*         cl = clients_[(*itr_r).clt_];
			if (cl) {
				if (cl->call(rlock_protocol::revoke, lid, (*itr_r).seq_, (*itr_r).revoke_type_, unused)
					!= rlock_protocol::OK) 
				{
					dbg_log(DBG_ERROR, "failed to send revoke\n");
				}
			} else {
				dbg_log(DBG_ERROR, "client %d didn't subscribe\n", clt);
			}
		}

		usleep(500);
	}
}


/// This method is a continuous loop, waiting for locks to be released
/// and then sending retry messages to those who are waiting for it.
void
LockManager::retryer()
{

	while (true) {
		pthread_mutex_lock(mutex_);
		while (available_locks_.empty()) {
			pthread_cond_wait(&available_cv_, mutex_);
		}
		lock_protocol::LockId lid = available_locks_.front();
		// XXX warning: this is not fault-tolerant
		available_locks_.pop_front();
		Lock* l = locks_[lid];
		std::deque<ClientRecord>& wq = l->waiting_list_;
		ClientRecord* cr = NULL;
		if (!wq.empty()) {
			// Verify the next waiting client can indeed grab the lock,
			// that is we don't have any false alarm arising from a race.
			cr = &wq.front();
			if (l->gtque_.CanGrant(cr->mode())) {
				l->expected_clt_ = cr->id();
				wq.pop_front();
			} else {
				cr = NULL;
			}
		}
		pthread_mutex_unlock(mutex_);

		if (cr) {
			int accepted;
			// TODO place a time limit on the retry for this client
			if (clients_[cr->id()]->call(rlock_protocol::retry, lid, cr->seq_,
				accepted) == rlock_protocol::OK) 
			{
				dbg_log(DBG_INFO,
				        "successfully sent a retry to clt %d seq %d for lck %llu\n",
				        cr->id(), cr->seq_, lid); 
				// client ignored retry. make the lock available for the next waiting
				// client
				if (!accepted) {
					pthread_mutex_lock(mutex_);
					l->expected_clt_ = -1;
					if (!wq.empty()) {
						available_locks_.push_back(lid);
					}
					pthread_mutex_unlock(mutex_);
				}
			} else {
				dbg_log(DBG_ERROR,
				        "failed to tell client %d to retry lock %llu\n", 
				        cr->id(), lid);
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
				// File ISSUE
			}
		}
		//usleep(500);
	}
}


} // namespace server
