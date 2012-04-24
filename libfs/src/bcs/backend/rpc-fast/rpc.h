#ifndef __RPCFAST_RPC_H
#define __RPCFAST_RPC_H

#include <list>
#include <map>
#include <vector>

#include "bcs/main/common/cdebug.h"

#include "rpc_signal.h"

#include "rpcs_types.h"
#include "marshall.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "rpcfastconfig.h"

using namespace std;

namespace rpcfast {

class rpc_const {
	public: 
                //ALL CONSTANTS RESERVED FOR INTERNAL USE (< 0)
                static const int failure = -1; 
		static const int unmarshal_args_failure = -2;
		static const int unmarshal_reply_failure = -3;
		static const int bind = -5;   // handler number reserved for bind
		static const int bind_failure = -6;
};

class fast_rpc {
 protected:
  typedef struct timespec timespec;
  typedef enum signal_status_t {
    UNUSED = 0,
    INUSE_NOCONT,
    INUSE_CONT
  } signal_status_t;

  typedef struct rpc_ticket_lock {
    volatile unsigned ticket; //only the ticket is on the client side
    unsigned dummy[3];
  } rpc_sync_t;
  //now serving is local to the server

  typedef struct rpc_signal_wait {
    volatile unsigned signal; // 1W
    signal_status_t status; // 1W
    unsigned long offset; // 2W
  } rpc_signal_wait_t;


  typedef struct rpc_send_msg {
    volatile unsigned signal;
    unsigned checksum;
    unsigned sizeInChar;
    //FIXME: Fixed for now
    char data[MAX_BUFF_SIZE]; // num of cache lines + 1 word
  } rpc_msg_t;

  typedef struct rpc_server_response {
    int ret;
    char resp[80 /* MAX_BUFF_SIZE - 1 WL (for the previous int)*/];
  } rpc_resp_t;

  /////////// METHODS START HERE /////////////
  timespec diff(timespec start, timespec end)
  {
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
      temp.tv_sec = end.tv_sec-start.tv_sec-1;
      temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
      temp.tv_sec = end.tv_sec-start.tv_sec;
      temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
  }

  inline unsigned getMaxChannelSize() {
    return (sizeof(rpc_signal_wait_t) + sizeof(rpc_msg_t)) * (MAX_TICKETS + 1);
  }

  void* map_shared_file(string filename, int* fdret, int type = OTHER) {
    void* channel;
    unsigned filesize;

    int fd = open(&filename[0], OPEN_MODE, PERM);

    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Map shared file %s\n", filename.c_str());

    if(fd < 0) {
      DBG_LOG(DBG_ERROR, DBG_MODULE(rpc), "%d: error opening file %s\n", fd, filename.c_str());
      return NULL;
    }

    filesize = type == BINDER? getMaxChannelSize() : MAX_SH_FILE_SIZE;

    channel =  mmap (0, filesize, PROT_READ | PROT_WRITE,
		     MAP_SHARED, fd, 0);
    if(channel == MAP_FAILED) {
      DBG_LOG(DBG_ERROR, DBG_MODULE(rpc), "error mapping file\n");
      return NULL;
    }

    assert(channel != NULL);

    if(fdret != NULL)
      *fdret = fd;

    return channel;

  }

  unsigned  calcChksum(char* data, unsigned sizeInChar) {
    unsigned chksum = 0, i;

    for(i = 0; i < sizeInChar; i++)
      chksum ^= data[i];

    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Checksum of %s is %d\n", data, chksum);

    return chksum;
  }

  int chkChksum(char* data, unsigned sizeInChar, unsigned orig_chksum) {
    unsigned running_chksum;
    running_chksum = calcChksum(data, sizeInChar);

    return ( running_chksum == orig_chksum ? 1 : 0);

  }

  inline rpc_msg_t* getQHead(rpc_sync_t* rpc_comm) {
    rpc_signal_wait_t* rpc_sig = (rpc_signal_wait_t*) &(rpc_comm[1]);
    return (rpc_msg_t*) &(rpc_sig[MAX_TICKETS+1]);
  }

  inline void rpcwait_incoming(volatile unsigned* sh_chan) {
  
    //spin on the signal
    while(*sh_chan != 1)
      __asm__ volatile ("rep;nop" ::: "memory");

  }

  int rpc_bind(string binder, char* client_sh_ch, bool unbind = false) {

    int fd, sig = 1;
    rpc_signal_wait_t* rpc_sig =  (rpc_signal_wait_t*) map_shared_file(binder, &fd);

    if(rpc_sig == NULL)
      return -1;

    rpc_msg_t* rpc_msg = (rpc_msg_t*) &rpc_sig[1];

    volatile unsigned* sh_chan = &(rpc_sig->signal);

    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Registration: Going to spin ...\n");

    //simple spin lock
    if(unbind)
      sig = 2;

    while(!__sync_bool_compare_and_swap(&(rpc_sig->signal), 0, sig));

    //send rpc register request to server
    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Got the lock!\n");
  
    if(unbind) {
      strcpy(rpc_msg->data, client_sh_ch);
      rpc_msg->signal = 1; //ACK to server
      close(fd); 
      DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "CLIENT UNBOUND successful!\n");
      return 0; //client done! bye!
    }

    while(rpc_msg->signal!=1);

    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Got reply from server!\n");

    assert(rpc_msg->signal == 1);
  
    strcpy(client_sh_ch, rpc_msg->data);

    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "File to load is: %s\n", client_sh_ch);

    rpc_msg->signal = 2; //ACK to server

    close(fd);

    return 0;
  }

  inline unsigned enqueue_send_and_wait(rpc_sync_t* rpc_sync) {
    unsigned myticket;
    myticket = __sync_fetch_and_add(&(rpc_sync->ticket), 1);
    if(myticket == MAX_TICKETS - 1)
      __sync_fetch_and_sub(&(rpc_sync->ticket), MAX_TICKETS);
    myticket = myticket % MAX_TICKETS;
    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "My Ticket: %d\n", myticket);

    rpc_signal_wait_t* rpc_signal = (rpc_signal_wait_t*) &rpc_sync[1];

    switch(rpc_signal[myticket].status) {
    case UNUSED:
      assert(__sync_bool_compare_and_swap(&(rpc_signal[myticket].status), UNUSED, INUSE_NOCONT));
      //rpc_signal[myticket].status = INUSE_NOCONT;
      break;
    case INUSE_NOCONT:
      //rpc_signal[myticket].status = INUSE_CONT;
      //should always succeed - if didn't then there is a really really bad race 
      assert(__sync_bool_compare_and_swap(&(rpc_signal[myticket].status), INUSE_NOCONT, INUSE_CONT));
      //assert( 0 && "Already INUSE_NOCONT and contending..");
      break;
    case INUSE_CONT:
      //already contending, abandon the ticket
      assert( 0 && "Already INUSE_CONT and DOUBLE contending..");
      break;
    default:
      assert(UNREACHED && "Unknown signal status");
    } 


    rpcwait_incoming(&(rpc_signal[myticket].signal)); //spin on signal
    
	DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Got signal!\n");

    //rpc_signal[myticket].signal = 0; //reset the signal
    return myticket;
  }

  int sendMsg(rpc_sync_t* rpc_sync,  char* msg, unsigned sizeInChar, char* &ret_buf, int &ret_sz) {
 
    unsigned myticket;
    myticket = enqueue_send_and_wait(rpc_sync);
    rpc_signal_wait_t* rpc_signal = (rpc_signal_wait_t*) &rpc_sync[1];

    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Client: %d, going to send sth!\n", myticket);

    rpc_msg_t* mybuff = getQHead(rpc_sync);
    mybuff = &(mybuff[myticket /* rpc_signal[ticket]->offset */]);
  
    rpc_signal[myticket].signal = 0; //reset the signal
    rpc_signal[myticket].status = UNUSED;
  
    assert(sizeInChar <= MAX_BUFF_SIZE);
  
    //while(mybuff->signal != 0); //wait for it to become zero just in case

    memcpy(mybuff->data, msg, sizeInChar);
    mybuff->sizeInChar = sizeInChar;
    mybuff->checksum = calcChksum(msg, sizeInChar);
  
    if(mybuff->signal == 0) { //a dummy load, makes if fine! :-| 
      assert(mybuff->signal == 0);
    }

    mybuff->signal = 1;

    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "SENT! %d\n", myticket);

    //FIXME: change it with xchg based wait...
    while(mybuff->signal != 2); //spin on the signal

    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "%d: got the response!\n", myticket);

    __sync_synchronize();
    mybuff->signal = 0;

    if(chkChksum(mybuff->data, mybuff->sizeInChar, calcChksum(mybuff->data, mybuff->sizeInChar))) {
      ret_sz = mybuff->sizeInChar;
      ret_buf = (char*) malloc (ret_sz * sizeof(char));
      mybuff->data[ret_sz] = '\0';
      memcpy(ret_buf, mybuff->data, mybuff->sizeInChar);
      DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Checksum matches! success!\n");
      return 0;
    }
    else {
      //checksum is wrong!!
      ret_buf = NULL;
      ret_sz = 0;
      cout << "Client: Error with reponse from server\n";
      return -1;
    }
  }

  int pin_to_core(int core);
};



// rpc client endpoint.
class rpcc : public fast_rpc  {

	private:

		bool bind_done_;
		string binder_shf;
		string sh_chan_shf;
		rpc_sync_t* sh_chan;		
		unsigned long client_id;
	public:

		rpcc(string);
		~rpcc();

		int bind();
		int unbind();
		int call1(unsigned int proc, 
				marshall &req, unmarshall &rep);

		unsigned id() { return client_id;}

		template<class R>
			int call_m(unsigned int proc, marshall &req, R & r);

		template<class R>
			int call(unsigned int proc, R & r); 
		template<class R, class A1>
			int call(unsigned int proc, const A1 & a1, R & r); 
		template<class R, class A1, class A2>
			int call(unsigned int proc, const A1 & a1, const A2 & a2, R & r); 
		template<class R, class A1, class A2, class A3>
			int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3, 
					R & r); 
		template<class R, class A1, class A2, class A3, class A4>
			int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3, 
					const A4 & a4, R & r);
		template<class R, class A1, class A2, class A3, class A4, class A5>
			int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3, 
					const A4 & a4, const A5 & a5, R & r); 
		template<class R, class A1, class A2, class A3, class A4, class A5,
			class A6>
				int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3, 
						const A4 & a4, const A5 & a5, const A6 & a6,
						R & r); 
		template<class R, class A1, class A2, class A3, class A4, class A5, 
			class A6, class A7>
				int call(unsigned int proc, const A1 & a1, const A2 & a2, const A3 & a3, 
						const A4 & a4, const A5 & a5, const A6 &a6, const A7 &a7,
						R & r); 

};

template<class R> int 
rpcc::call_m(unsigned int proc, marshall &req, R & r) 
{
	unmarshall u;
	int intret = call1(proc, req, u);
	if (intret < 0) return intret;
	u >> r;
	if(u.okdone() != true)
		return rpc_const::unmarshal_reply_failure;
	return intret;
}

template<class R> int
rpcc::call(unsigned int proc, R & r) 
{
	marshall m;
	return call_m(proc, m, r);
}

template<class R, class A1> int
rpcc::call(unsigned int proc, const A1 & a1, R & r) 
{
	marshall m;
	m << a1;
	return call_m(proc, m, r);
}

template<class R, class A1, class A2> int
rpcc::call(unsigned int proc, const A1 & a1, const A2 & a2,
		R & r) 
{
	marshall m;
	m << a1;
	m << a2;
	return call_m(proc, m, r);
}

template<class R, class A1, class A2, class A3> int
rpcc::call(unsigned int proc, const A1 & a1, const A2 & a2,
		const A3 & a3, R & r) 
{
	marshall m;
	m << a1;
	m << a2;
	m << a3;
	return call_m(proc, m, r);
}

template<class R, class A1, class A2, class A3, class A4> int
rpcc::call(unsigned int proc, const A1 & a1, const A2 & a2,
		const A3 & a3, const A4 & a4, R & r) 
{
	marshall m;
	m << a1;
	m << a2;
	m << a3;
	m << a4;
	return call_m(proc, m, r);
}

template<class R, class A1, class A2, class A3, class A4, class A5> int
rpcc::call(unsigned int proc, const A1 & a1, const A2 & a2,
		const A3 & a3, const A4 & a4, const A5 & a5, R & r) 
{
	marshall m;
	m << a1;
	m << a2;
	m << a3;
	m << a4;
	m << a5;
	return call_m(proc, m, r);
}

template<class R, class A1, class A2, class A3, class A4, class A5,
	class A6> int
rpcc::call(unsigned int proc, const A1 & a1, const A2 & a2,
		const A3 & a3, const A4 & a4, const A5 & a5, 
		const A6 & a6, R & r) 
{
	marshall m;
	m << a1;
	m << a2;
	m << a3;
	m << a4;
	m << a5;
	m << a6;
	return call_m(proc, m, r);
}

template<class R, class A1, class A2, class A3, class A4, class A5,
	class A6, class A7> int
rpcc::call(unsigned int proc, const A1 & a1, const A2 & a2,
		const A3 & a3, const A4 & a4, const A5 & a5, 
		const A6 & a6, const A7 & a7,
		R & r) 
{
	marshall m;
	m << a1;
	m << a2;
	m << a3;
	m << a4;
	m << a5;
	m << a6;
	m << a7;
	return call_m(proc, m, r);
}

class handler {
	public:
		handler() { }
		virtual ~handler() { }
		virtual int fn(unmarshall &, marshall &) = 0;
};


// rpc server endpoint.
class rpcs : public fast_rpc {
	typedef enum {
		NEW,  // new RPC, not a duplicate
		INPROGRESS, // duplicate of an RPC we're still processing
		DONE, // duplicate of an RPC we already replied to (have reply)
		FORGOTTEN,  // duplicate of an old RPC whose reply we've forgotten
	} rpcstate_t;

	private:
	//fast rpc server types
	typedef struct send_queue {
	  rpc_msg_t* rpc_request;
	  unsigned long long tstamp;// FIXME: not used for the timestamp, yet!
	  unsigned client_ticket;
	  timespec start;
	} send_queue_t;


	typedef enum server_queue_state {
	  UNLOCKED = 0,
	  LOCKED
	} s_q_stat_t;

	typedef struct server_lock {
#ifdef RPCS_VER_1
	  s_q_stat_t lock_queue;
#endif
	  unsigned int client_id;
	  int fd;
	  unsigned now_serving_rpc_comm;
	  unsigned num_expected;
	  unsigned num_served;
	  rpc_sync_t* rpc_comm_l;
	  rpc_signal_wait_t* rpc_comm_sig;
	  rpc_msg_t* head_send_q;
	  vector<send_queue_t*> sendq;
	  unsigned long long gtstamp;
	} server_lock_t;

	typedef struct rpc_registry {
	  rpc_signal_wait_t* rpc_reg_lock;
	  rpc_msg_t* rpc_reg_reply;
	  vector<server_lock_t*> rpc_queue;
	  unsigned long tot_served[NUMTHREADS];
	} rpc_registry_t;

#define MAX_LOG_FILE_SIZE_PT MAX_LOG_FILE_SIZE/NUMTHREADS

	struct server_log {
	  char* file;
	  unsigned len[NUMTHREADS];
	  unsigned long num_locked[NUMTHREADS];
	  unsigned long spl_num_locked[NUMTHREADS];
	  unsigned long num_cycles[NUMTHREADS];
	};

	//fast rpc server types -- ENDS

	struct reply_t {
 		reply_t () {
			buf = NULL;
			sz = 0;
		}
		char *buf;
		int sz;
	};

	// map proc # to function
	std::map<int, handler *> procs_;
	
	//main server registry
	rpc_registry_t rpc_reg;
 
	server_log rpc_log;
	//fast rpc helper-private methods

	void handle_comm_q(server_lock_t* serv);
	void handle_send_q(server_lock_t* rpc_serv, unsigned tid);
	void check_registry_incoming();

	void mt_log_service(unsigned id, timespec start);

	void* map_shared_file_server(string filename, int * fdret, int type = OTHER);
	void sanity_check();
	void init_rpc_registry(const char* sh_file);
	void init_rpc(char* filename, server_lock_t* ne);

	std::string bind_f_;

	protected:

	// internal handler registration
	void reg1(unsigned int proc, handler *);

	public:
	//FIXME - SHOULD BE MADE PRIVATE/PROTECTED - SHOULDNT BE PUBLIC
	void dispatch(char* buf, unsigned int &sz /*return buffer is the same*/);
	
	//should be called in the user's server class after registering all its services
	int main_service_loop(const char* sh_file);
	int main_service_loop();

	void *rpc_server_kernel(void* arg);

	rpcs(const char* bind_file);
	~rpcs();

	//RPC handler for clients binding
	int rpcbind(int a, int &r);

	// register a handler
	template<class S, class A1, class R>
		void reg(unsigned int proc, S*, int (S::*meth)(const A1 a1, R & r));
	template<class S, class A1, class A2, class R>
		void reg(unsigned int proc, S*, int (S::*meth)(const A1 a1, const A2, 
					R & r));
	template<class S, class A1, class A2, class A3, class R>
		void reg(unsigned int proc, S*, int (S::*meth)(const A1, const A2, 
					const A3, R & r));
	template<class S, class A1, class A2, class A3, class A4, class R>
		void reg(unsigned int proc, S*, int (S::*meth)(const A1, const A2, 
					const A3, const A4, R & r));
	template<class S, class A1, class A2, class A3, class A4, class A5, class R>
		void reg(unsigned int proc, S*, int (S::*meth)(const A1, const A2, 
					const A3, const A4, const A5, 
					R & r));
	template<class S, class A1, class A2, class A3, class A4, class A5, class A6,
		class R>
			void reg(unsigned int proc, S*, int (S::*meth)(const A1, const A2, 
						const A3, const A4, const A5, 
						const A6, R & r));
	template<class S, class A1, class A2, class A3, class A4, class A5, class A6,
		class A7, class R>
			void reg(unsigned int proc, S*, int (S::*meth)(const A1, const A2, 
						const A3, const A4, const A5, 
						const A6, const A7,
						R & r));
};

template<class S, class A1, class R> void
rpcs::reg(unsigned int proc, S*sob, int (S::*meth)(const A1 a1, R & r))
{
	class h1 : public handler {
		private:
			S * sob;
			int (S::*meth)(const A1 a1, R & r);
		public:
			h1(S *xsob, int (S::*xmeth)(const A1 a1, R & r))
				: sob(xsob), meth(xmeth) { }
			int fn(unmarshall &args, marshall &ret) {
				A1 a1;
				R r;
				args >> a1;
				if(!args.okdone())
					return rpc_const::unmarshal_args_failure;
				int b = (sob->*meth)(a1, r);
				ret << r;
				return b;
			}
	};
	reg1(proc, new h1(sob, meth));
}

template<class S, class A1, class A2, class R> void
rpcs::reg(unsigned int proc, S*sob, int (S::*meth)(const A1 a1, const A2 a2, 
			R & r))
{
	class h1 : public handler {
		private:
			S * sob;
			int (S::*meth)(const A1 a1, const A2 a2, R & r);
		public:
			h1(S *xsob, int (S::*xmeth)(const A1 a1, const A2 a2, R & r))
				: sob(xsob), meth(xmeth) { }
			int fn(unmarshall &args, marshall &ret) {
				A1 a1;
				A2 a2;
				R r;
				args >> a1;
				args >> a2;
				if(!args.okdone())
					return rpc_const::unmarshal_args_failure;
				int b = (sob->*meth)(a1, a2, r);
				ret << r;
				return b;
			}
	};
	reg1(proc, new h1(sob, meth));
}

template<class S, class A1, class A2, class A3, class R> void
rpcs::reg(unsigned int proc, S*sob, int (S::*meth)(const A1 a1, const A2 a2, 
			const A3 a3, R & r))
{
	class h1 : public handler {
		private:
			S * sob;
			int (S::*meth)(const A1 a1, const A2 a2, const A3 a3, R & r);
		public:
			h1(S *xsob, int (S::*xmeth)(const A1 a1, const A2 a2, const A3 a3, R & r))
				: sob(xsob), meth(xmeth) { }
			int fn(unmarshall &args, marshall &ret) {
				A1 a1;
				A2 a2;
				A3 a3;
				R r;
				args >> a1;
				args >> a2;
				args >> a3;
				if(!args.okdone())
					return rpc_const::unmarshal_args_failure;
				int b = (sob->*meth)(a1, a2, a3, r);
				ret << r;
				return b;
			}
	};
  reg1(proc, new h1(sob, meth));
}

template<class S, class A1, class A2, class A3, class A4, class R> void
rpcs::reg(unsigned int proc, S*sob, int (S::*meth)(const A1 a1, const A2 a2, 
			const A3 a3, const A4 a4, 
			R & r))
{
	class h1 : public handler {
		private:
			S * sob;
			int (S::*meth)(const A1 a1, const A2 a2, const A3 a3, const A4 a4, R & r);
		public:
			h1(S *xsob, int (S::*xmeth)(const A1 a1, const A2 a2, const A3 a3, 
						const A4 a4, R & r))
				: sob(xsob), meth(xmeth)  { }
			int fn(unmarshall &args, marshall &ret) {
				A1 a1;
				A2 a2;
				A3 a3;
				A4 a4;
				R r;
				args >> a1;
				args >> a2;
				args >> a3;
				args >> a4;
				if(!args.okdone())
					return rpc_const::unmarshal_args_failure;
				int b = (sob->*meth)(a1, a2, a3, a4, r);
				ret << r;
				return b;
			}
	};
	reg1(proc, new h1(sob, meth));
}

template<class S, class A1, class A2, class A3, class A4, class A5, class R> void
rpcs::reg(unsigned int proc, S*sob, int (S::*meth)(const A1 a1, const A2 a2, 
			const A3 a3, const A4 a4, 
			const A5 a5, R & r))
{
	class h1 : public handler {
		private:
			S * sob;
			int (S::*meth)(const A1 a1, const A2 a2, const A3 a3, const A4 a4, 
					const A5 a5, R & r);
		public:
			h1(S *xsob, int (S::*xmeth)(const A1 a1, const A2 a2, const A3 a3, 
						const A4 a4, const A5 a5, R & r))
				: sob(xsob), meth(xmeth) { }
			int fn(unmarshall &args, marshall &ret) {
				A1 a1;
				A2 a2;
				A3 a3;
				A4 a4;
				A5 a5;
				R r;
				args >> a1;
				args >> a2;
				args >> a3;
				args >> a4;
				args >> a5;
				if(!args.okdone())
					return rpc_const::unmarshal_args_failure;
				int b = (sob->*meth)(a1, a2, a3, a4, a5, r);
				ret << r;
				return b;
			}
	};
	reg1(proc, new h1(sob, meth));
}

template<class S, class A1, class A2, class A3, class A4, class A5, class A6, class R> void
rpcs::reg(unsigned int proc, S*sob, int (S::*meth)(const A1 a1, const A2 a2, 
			const A3 a3, const A4 a4, 
			const A5 a5, const A6 a6, 
			R & r))
{
	class h1 : public handler {
		private:
			S * sob;
			int (S::*meth)(const A1 a1, const A2 a2, const A3 a3, const A4 a4, 
					const A5 a5, const A6 a6, R & r);
		public:
			h1(S *xsob, int (S::*xmeth)(const A1 a1, const A2 a2, const A3 a3, 
						const A4 a4, const A5 a5, const A6 a6, R & r))
				: sob(xsob), meth(xmeth) { }
			int fn(unmarshall &args, marshall &ret) {
				A1 a1;
				A2 a2;
				A3 a3;
				A4 a4;
				A5 a5;
				A6 a6;
				R r;
				args >> a1;
				args >> a2;
				args >> a3;
				args >> a4;
				args >> a5;
				args >> a6;
				if(!args.okdone())
					return rpc_const::unmarshal_args_failure;
				int b = (sob->*meth)(a1, a2, a3, a4, a5, a6, r);
				ret << r;
				return b;
			}
	};

	reg1(proc, new h1(sob, meth));
}

template<class S, class A1, class A2, class A3, class A4, class A5, 
	class A6, class A7, class R> void
rpcs::reg(unsigned int proc, S*sob, int (S::*meth)(const A1 a1, const A2 a2, 
			const A3 a3, const A4 a4, 
			const A5 a5, const A6 a6,
			const A7 a7, R & r))
{
	class h1 : public handler {
		private:
			S * sob;
			int (S::*meth)(const A1 a1, const A2 a2, const A3 a3, const A4 a4, 
					const A5 a5, const A6 a6, const A7 a7, R & r);
		public:
			h1(S *xsob, int (S::*xmeth)(const A1 a1, const A2 a2, const A3 a3, 
						const A4 a4, const A5 a5, const A6 a6,
						const A7 a7, R & r))
				: sob(xsob), meth(xmeth) { }
			int fn(unmarshall &args, marshall &ret) {
				A1 a1;
				A2 a2;
				A3 a3;
				A4 a4;
				A5 a5;
				A6 a6;
				A7 a7;
				R r;
				args >> a1;
				args >> a2;
				args >> a3;
				args >> a4;
				args >> a5;
				args >> a6;
				args >> a7;
				if(!args.okdone())
					return rpc_const::unmarshal_args_failure;
				int b = (sob->*meth)(a1, a2, a3, a4, a5, a6, a7, r);
				ret << r;
				return b;
			}
	};

	reg1(proc, new h1(sob, meth));
}


int cmp_timespec(const struct timespec &a, const struct timespec &b);
void add_timespec(const struct timespec &a, int b, struct timespec *result);
int diff_timespec(const struct timespec &a, const struct timespec &b);

} // namespace rpcfast

#endif // __RPCFAST_RPC_H 
