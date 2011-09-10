// the caching lock server implementation

#include "server/lckmgr.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common/debug.h"

namespace server {


#if 0
static int table_mode[][] = {
	/*                                    Requested Lock Mode                             */
	/*                SL,           SR,           XL,           XR,       IS,           IX,           IXSL            */
	/* Status   */ 
	/* FREE     */ {  Lock::SL,     Lock::SR,     Lock::XL,     Lock::XR, Lock::IS,     Lock::IX,     Lock::IXSL },
	/* SL       */ {  Lock::SL,     Lock::SLSR,   Lock::NA,     Lock::NA, Lock::ISSL,   Lock::IXSL,   Lock::IXSL },
	/* SR       */ {  Lock::SLSR,   Lock::SR,     Lock::NA,     Lock::NA, Lock::ISSR,   Lock::NA,     Lock::NA },
	/* SLSR     */ {  Lock::SLSR,   Lock::SLSR,   Lock::NA,     Lock::NA, Lock::ISSLSR, Lock::NA,     Lock::NA },
	/* IS       */ {  Lock::ISSL,   Lock::ISSR,   Lock::ISXL,   Lock::NA, Lock::IS,     Lock::ISIX,   Lock::ISIXSL },
	/* IX       */ {  Lock::IXSL,   Lock::NA,     Lock::IXXL,   Lock::NA, Lock::ISIX,   Lock::IX,     Lock::IXSL },
	/* XL       */ {  Lock::NA,     Lock::NA,     Lock::NA,     Lock::NA, Lock::ISXL,   Lock::IXXL,   Lock::NA },
	/* XR       */ {  Lock::NA,     Lock::NA,     Lock::NA,     Lock::NA, Lock::NA,     Lock::NA,     Lock::NA },
	/* ISSL     */ {  Lock::ISSL,   Lock::ISSLSR, Lock::NA,     Lock::NA, Lock::ISSL,   Lock::ISIXSL, Lock::ISIXSL },
	/* ISSLSR   */ {  Lock::ISSLSR, Lock::ISSLSR, Lock::NA,     Lock::NA, Lock::ISSLSR, Lock::NA,     Lock::NA },
	/* ISSR     */ {  Lock::ISSLSR, Lock::ISSR,   Lock::NA,     Lock::NA, Lock::ISSR,   Lock::NA,     Lock::NA },
	/* ISXL     */ {  Lock::NA,     Lock::NA,     Lock::NA,     Lock::NA, Lock::ISXL,   Lock::ISIXXL, Lock::NA },
	/* ISIX     */ {  Lock::ISIXSL, Lock::ISIX,   Lock::ISIXXL, Lock::NA, Lock::ISIX,   Lock::ISIX,   Lock::ISIXSL },
	/* ISIXXL   */ {  Lock::NA,     Lock::NA,     Lock::NA,     Lock::NA, Lock::ISIXXL, Lock::ISIXXL, Lock::NA },
	/* ISIXSL   */ {  Lock::ISIXSL, Lock::NA,     Lock::NA,     Lock::NA, Lock::ISIXSL, Lock::ISIXSL, Lock::ISIXSL },
	/* IXSL     */ {  Lock::IXSL,   Lock::NA,     Lock::NA,     Lock::NA, Lock::ISIXSL, Lock::IXSL,   Lock::IXSL },
	/* IXXL     */ {  Lock::NA,     Lock::NA,     Lock::NA,     Lock::NA, Lock::ISIXXL, Lock::IXXL,   Lock::NA },

#endif 


static std::string mode2str[] = { "SL", "SR", "IS", "IX", "XL", "XR", "IXSL"};

static bool compatibility_table[8][8] = {
	/*                FREE, SL,    SR,    XL,    XR,    IS,    IX,    IXSL   */
	/* FREE     */ {  true, true,  true,  true,  true,  true,  true,  true },
	/* SL       */ {  true, true,  true,  false, false, true,  true,  true },
	/* SR       */ {  true, true,  true,  false, false, true,  false, false },
	/* IS       */ {  true, true,  true,  true,  false, true,  true,  true },
	/* IX       */ {  true, true,  false, true,  false, true,  true,  true },
	/* XL       */ {  true, false, false, false, false, true,  true,  false },
	/* XR       */ {  true, false, false, false, false, false, false, false },
	/* IXSL     */ {  true, true,  false, false, false, true,  true,  true }
};


static int revoke_table[8][8] = {
	/*                                                              Requested Lock Mode                                                            */
	/*                FREE,         SL,           SR,           XL,                XR,           IS,              IX,              IXSL            */
	/* FREE     */ {  Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NO,      Lock::RVK_NO, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NO },
	/* SL       */ {  Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NL,      Lock::RVK_NL, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NO },
	/* SR       */ {  Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NL,      Lock::RVK_NL, Lock::RVK_NO,    Lock::RVK_SR2SL, Lock::RVK_SR2SL },
	/* IS       */ {  Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NO,      Lock::RVK_NL, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NO },
	/* IX       */ {  Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NL, Lock::RVK_NO,      Lock::RVK_NL, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NO },
	/* XL       */ {  Lock::RVK_NO, Lock::RVK_NL, Lock::RVK_NL, Lock::RVK_NL,      Lock::RVK_NL, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_XL2SL },
	/* XR       */ {  Lock::RVK_NO, Lock::RVK_NL, Lock::RVK_NL, Lock::RVK_NL,      Lock::RVK_NL, Lock::RVK_XR2XL, Lock::RVK_XR2XL, Lock::RVK_NL },
	/* IXSL     */ {  Lock::RVK_NO, Lock::RVK_NO, Lock::RVK_NL, Lock::RVK_IXSL2IX, Lock::RVK_NL, Lock::RVK_NO,    Lock::RVK_NO,    Lock::RVK_NO },
};


int revoke2mode_table[] = {
	/* RVK_NO      */  -1,
	/* RVK_NL      */  lock_protocol::FREE,
	/* RVK_XL2SL   */  lock_protocol::SL,
	/* RVK_SR2SL   */  lock_protocol::SL,
	/* RVK_XR2XL   */  lock_protocol::XL,
	/* RVK_IXSL2IX */  lock_protocol::IX
};

ClientRecord::ClientRecord()
	: clt_(-1), 
	  seq_(-1),
	  mode_(lock_protocol::NONE)

{

}


ClientRecord::ClientRecord(int clt, int seq, int mode)
	: clt_(clt), 
	  seq_(seq),
	  mode_(mode)
{

}


Lock::Lock()
	: status_(0),
	  expected_clt_(-1), 
	  retry_responded_(false), 
	  revoke_sent_(false)
{
	pthread_cond_init(&retry_responded_cv_, NULL);
	for (int i=0; i<lock_protocol::IXSL+1; i++) {
		mode_cnt_[i] = 0;
	}
}


Lock::~Lock()
{
	pthread_cond_destroy(&retry_responded_cv_);
}


bool
Lock::IsModeCompatible(int mode)
{
	int val = status_;
	int m;

	while (val) {
		m = __builtin_ctz(val); 
		val &= ~(1 << m);
		if (!compatibility_table[m][mode]) { 
			return false;
		}
	}
	return true;
}


void
Lock::AddHolderAndUpdateStatus(const ClientRecord& cr)
{
	assert(holders_.find(cr.clt_) == holders_.end()); // a holder should use convert
	holders_[cr.clt_] = cr;
	mode_cnt_[cr.mode_]++;
	status_ |= 1 << cr.mode_;
}


void
Lock::RemoveHolderAndUpdateStatus(int clt)
{
	assert(holders_.find(clt) != holders_.end());
	ClientRecord& cr = holders_[clt];
	assert(mode_cnt_[cr.mode_]>0);
	if (--mode_cnt_[cr.mode_] == 0) {
		status_ &= ~(1 << cr.mode_);
	}
	holders_.erase(clt);
}


void
Lock::ConvertHolderAndUpdateStatus(int clt, int new_mode)
{
	assert(holders_.find(clt) != holders_.end());
	ClientRecord& cr = holders_[clt];
	assert(mode_cnt_[cr.mode_]>0);
	if (--mode_cnt_[cr.mode_] == 0) {
		status_ &= ~(1 << cr.mode_);
	}
	cr.mode_ = new_mode;
	mode_cnt_[cr.mode_]++;
	status_ |= 1 << cr.mode_;
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


	
static std::string
state2str(uint32_t state)
{
	int         m;
	std::string str;
	
	while (state) {
		m = __builtin_ctz(state); 
		state &= ~(1 << m);
		str+=mode2str[m];
	}
	return str;
}



lock_protocol::status
LockManager::acquire(int clt, int seq, lock_protocol::LockId lid, int mode, int flags,
                     int &unused)
{
	char                  statestr[128];
	uint32_t              next_state;
	int                   queue_len;
	lock_protocol::status r;
	dbg_log(DBG_INFO, "clt %d seq %d acquiring lock %llu (%s)\n", clt, seq, 
	        lid, mode2str[mode].c_str());

	pthread_mutex_lock(&mutex_);
	Lock& l = locks_[lid];
	queue_len = l.waiting_list_.size();
	dbg_log(DBG_INFO, "queue len for lock %llu: %d\n", lid, queue_len);

    if ((queue_len == 0 || (queue_len > 0 && l.expected_clt_ == clt)) &&
		l.IsModeCompatible(mode)) 
	{
		dbg_log(DBG_INFO, "lock %llu is compatible (%s); granting to clt %d\n", 
		        lid, state2str(l.status_).c_str(), clt);
		r = lock_protocol::OK;
		l.AddHolderAndUpdateStatus(ClientRecord(clt, seq, mode));
		l.expected_clt_ = -1;
		if (queue_len != 0) {
			// Since there are clients waiting, we have two options. If the 
			// request of the next client in line is:
			// 1) compatible with the just serviced request, we send a retry 
			//    msg to the waiting client.
			// 2) incompatible with the just serviced request, we add the lock
			//    in the revoke set to get it back.
			std::deque<ClientRecord>& wq = l.waiting_list_;
			ClientRecord&             cr = wq.front();
			if (l.IsModeCompatible(cr.mode_)) {
				available_locks_.push_back(lid);
				pthread_cond_signal(&available_cv_);
			} else {
				revoke_set_.insert(lid);
				l.revoke_sent_ = true;
				pthread_cond_signal(&revoke_cv_);
			}
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
		l.waiting_list_.push_back(ClientRecord(clt, seq, mode));
		r = lock_protocol::RETRY;
	}
	pthread_mutex_unlock(&mutex_);
	return r;
}


lock_protocol::status
LockManager::release(int clt, int seq, lock_protocol::LockId lid, int rl_type, int& unused)
{
	std::map<int, ClientRecord>::iterator  itr_icr;
	lock_protocol::status                  r = lock_protocol::OK;
	pthread_mutex_lock(&mutex_);
	int                                    next_status;

	if (locks_.find(lid) != locks_.end() && 
	    locks_[lid].holders_.find(clt) != locks_[lid].holders_.end())
	{
		dbg_log(DBG_INFO, "clt %d released lck %llu at seq %d\n", clt, lid, seq);
		Lock& l = locks_[lid];
		assert(l.holders_[clt].seq_ == seq);
		if (rl_type == lock_protocol::RVK_NL) {
			l.RemoveHolderAndUpdateStatus(clt);
		} else {
			int new_mode = revoke2mode_table[rl_type];
			l.ConvertHolderAndUpdateStatus(clt, new_mode);
		}
		// Check whether the lock can be made available to the next waiting client.
		// But first ensure there is no outstanding acquire (expected_clt == -1), 
		// which expects to get the lock otherwise we could end up making 
		// the lock available to a second client. 
		if (l.expected_clt_ == -1) {
			std::deque<ClientRecord>& wq = l.waiting_list_;
			if (!wq.empty()) {
				ClientRecord& wcr = wq.front();
				if (l.IsModeCompatible(wcr.mode_)) {
					available_locks_.push_back(lid);
					pthread_cond_signal(&available_cv_);
				}
			}
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
	int                                       clt;
	int                                       revoke_type;

	while (true) {
		pthread_mutex_lock(&mutex_);
		while (revoke_set_.empty()) {
			pthread_cond_wait(&revoke_cv_, &mutex_);
		}
		itr_l = revoke_set_.begin();
		lid = *itr_l;
		revoke_set_.erase(lid);
		Lock& l = locks_[lid];
		ClientRecord& waiting_cr = l.waiting_list_.front();

		for (itr_icr = l.holders_.begin(); 
		     itr_icr != l.holders_.end(); itr_icr++) 
		{
			int           unused;
			int           clt = (*itr_icr).first;
			ClientRecord& cr = (*itr_icr).second;
			rpcc*         cl = clients_[clt];
			revoke_type = revoke_table[cr.mode_][waiting_cr.mode_];
			assert(revoke_type != Lock::RVK_NO);
			if (cl) {
				if (cl->call(rlock_protocol::revoke, lid, cr.seq_, revoke_type, unused)
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
		Lock& l = locks_[lid];
		std::deque<ClientRecord>& wq = l.waiting_list_;
		ClientRecord* cr = NULL;
		if (!wq.empty()) {
			// Verify the next waiting client can indeed grab the lock,
			// that is we don't have any false alarm arising from a race.
			cr = &wq.front();
			if (l.IsModeCompatible(cr->mode_)) {
				l.expected_clt_ = cr->clt_;
				wq.pop_front();
			} else {
				cr = NULL;
			}
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
				// File ISSUE
			}
		}
		//usleep(500);
	}
}

} // namespace server
