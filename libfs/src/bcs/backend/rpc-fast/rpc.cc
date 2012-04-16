#include "bcs/backend/rpc-fast/rpc.h"
#include "bcs/main/common/cdebug.h"

#include <time.h>
#include <string.h> //for strnlen
#include <vector>


using namespace std;

#define MAX_BIND_FILENAME_LEN 25

int fast_rpc::pin_to_core(int core) {
    cpu_set_t cpu_mask;

    CPU_ZERO(&cpu_mask);
    CPU_SET(core, &cpu_mask);

    if(sched_setaffinity(0, sizeof(cpu_mask), &cpu_mask) < 0) {
      DBG_LOG(DBG_ERROR, DBG_MODULE(rpc), "Error while setting the CPU affinity: core=%d\n", core);
      return -1;
    }
    return 0;
}


inline
void set_rand_seed()
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	srandom((int)ts.tv_nsec^((int)getpid()));
}

rpcc::rpcc(string bind_f) : 
  bind_done_(false),client_id(0)
{

  binder_shf = bind_f; 
  sh_chan = NULL;
}

rpcc::~rpcc()
{
  //free(binder_shf);
  //free(sh_chan_shf);
}

int
rpcc::bind()
{
  //FIXME: cleanup - make rpc_bind map the shared file than in here

  if(client_id != 0 || bind_done_) {
    cout << "rpc bind already done!\n";
    return -1;
  }

  char c_shf[MAX_BIND_FILENAME_LEN];
  int ret = rpc_bind(binder_shf, c_shf); //FAST_RPC CLIENT BIND
	
  //set the shared channel, shared file, etc etc, returned by the fast rpc register
  sh_chan = (rpc_sync_t*) map_shared_file(c_shf, NULL, OTHER);
  client_id = (unsigned) atoi(c_shf);
  sh_chan_shf = string(c_shf);

  if (ret == 0) {
    bind_done_ = true;
    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "rpcc bind successful, shared-file: %s, client id - %d\n", sh_chan_shf.c_str(), client_id);
  } else {
    DBG_LOG(DBG_ERROR, DBG_MODULE(rpc), "rpcc bind failed\n");
  }
  return ret;
}

int
rpcc::unbind()
{
  
  int ret = rpc_bind(binder_shf, &sh_chan_shf[0], true); //FAST_RPC CLIENT UNBIND REQUEST

#ifdef DEBUG	
  cout << " client unbound request \n";
#endif

  if (ret == 0) {
    bind_done_ = false;
    client_id = 0;
    sh_chan_shf = string("");
#ifdef DEBUG	
    cout << " rpcc unbind successful!!\n";
#endif
  } else {
    //jsl_log(JSL_DBG_2, "rpcc::bind failed %d\n",ret);
    cout << "rpcc unbind failed\n";
  }

  return ret;
}



int
rpcc::call1(unsigned int proc, marshall &req, unmarshall &rep)
{
  char* ret_buf = NULL;
  int ret_sz, ret;

#ifdef DEBUG	
  cout << "call to proc: " << proc << "\n";
#endif

  req_header h(client_id,proc);
  req.pack_req_header(h);
  ret = sendMsg(sh_chan, req.cstr(), req.size(), ret_buf, ret_sz);
  if(ret != 0) {
    //send msg didnt succeed
    cout << "send msg failed\n";
    return rpc_const::failure;
  }
  else {
    //succeeded sending the msg
    unmarshall tmp_rep(ret_buf, ret_sz);
    reply_header rep_h;
#ifdef DEBUG	
    cout << "send msg succeeded\n";
#endif
    tmp_rep.unpack_reply_header(&rep_h);
    assert(rep_h.client_id == client_id && "client id is not same as mine!");
    rep.take_in(tmp_rep);
    return rep_h.ret;
  }

}


rpcs::rpcs(const char* bind_f)
	: bind_f_(bind_f)
{
	set_rand_seed();

    DBG_LOG(DBG_INFO, DBG_MODULE(rpc), "created rpcs server object!\n");

	//main_service_loop(bind_f);

	//reg(rpc_const::bind, this, &rpcs::rpcbind); // we handled binding manually..
}

rpcs::~rpcs()
{
	//must delete listener before dispatchpool

}

void
rpcs::reg1(unsigned int proc, handler *h)
{
  DBG_LOG(DBG_INFO, DBG_MODULE(rpc), "registering for service... %u\n", proc);
  
  assert(procs_.count(proc) == 0);
  procs_[proc] = h;
  assert(procs_.count(proc) >= 1);
}

void
rpcs::dispatch(char* buf, unsigned int& sz /*return buffer is the same*/)
{
  unmarshall req(buf, sz);

  req_header h;
  req.unpack_req_header(&h);
  int proc = h.proc;

  if(!req.ok()) {
    //jsl_log(JSL_DBG_1, "rpcs:dispatch unmarshall header failed!!!\n");
    cout << "rpcs:dispatch unmarshall header failed!!!\n";
    return;
  }


  marshall rep;
  reply_header rh(0);

  handler *f;
  if(procs_.count(proc) < 1){
    //jsl_log(JSL_DBG_2, "rpcs::dispatch: bad proc %x procs.count %u\n",
    //	  proc, (unsigned)procs_.count(proc));
    cout << "rpcs::dispatch: bad proc: "<< proc << "\n";
    return;
  }

  f = procs_[proc];

  rh.ret = f->fn(req, rep);
  rh.client_id = h.client_id;
#ifdef DEBUG
  cout << "returned value is: " << rh.ret <<"\n";
#endif
  assert(rh.ret >= 0 || 
	 rh.ret == rpc_const::unmarshal_args_failure);

  rep.pack_reply_header(rh);
  char* buf1;
  int sz1;
	
  rep.take_buf(&buf1,&sz1);
  memcpy(buf, buf1, sz1);
  sz=sz1;
  free(buf1);
	
}


//rpc handler
int 
rpcs::rpcbind(int a, int &r)
{
  //UNUSED!!
  r = 0;
  cout << " new client binding is done\n";
  return 0;
}

void
marshall::rawbyte(unsigned char x)
{
	if (_ind >= _capa) {
		_capa *= 2;
		assert (_buf != NULL);
		_buf = (char *)realloc(_buf, _capa);
		assert(_buf);
	}
	_buf[_ind++] = x;
}

void
marshall::rawbytes(const char *p, int n)
{
	if ((_ind+n) > _capa) {
		_capa = _capa > n? 2*_capa:(_capa+n);
		assert (_buf != NULL);
		_buf = (char *)realloc(_buf, _capa);
		assert(_buf);
	}
	memcpy(_buf+_ind, p, n);
	_ind += n;
}

marshall &
operator<<(marshall &m, unsigned char x)
{
	m.rawbyte(x);
	return m;
}

marshall &
operator<<(marshall &m, char x)
{
	m << (unsigned char) x;
	return m;
}


marshall &
operator<<(marshall &m, unsigned short x)
{
	m.rawbyte((x >> 8) & 0xff);
	m.rawbyte(x & 0xff);
	return m;
}

marshall &
operator<<(marshall &m, short x)
{
	m << (unsigned short) x;
	return m;
}

marshall &
operator<<(marshall &m, unsigned int x)
{
	//network order is big-endian
	m.rawbyte((x >> 24) & 0xff);
	m.rawbyte((x >> 16) & 0xff);
	m.rawbyte((x >> 8) & 0xff);
	m.rawbyte(x & 0xff);
	return m;
}

marshall &
operator<<(marshall &m, int x)
{
	m << (unsigned int) x;
	return m;
}

marshall &
operator<<(marshall &m, const std::string &s)
{
	m << (unsigned int) s.size();
	m.rawbytes(s.data(), s.size());
	return m;
}

marshall &
operator<<(marshall &m, unsigned long long x)
{
	m << (unsigned int) (x >> 32);
	m << (unsigned int) x;
	return m;
}

void
marshall::pack(int x)
{
	rawbyte((x >> 24) & 0xff);
	rawbyte((x >> 16) & 0xff);
	rawbyte((x >> 8) & 0xff);
	rawbyte(x & 0xff);
}

void
unmarshall::unpack(int *x)
{
	(*x) = (rawbyte() & 0xff) << 24;
	(*x) |= (rawbyte() & 0xff) << 16;
	(*x) |= (rawbyte() & 0xff) << 8;
	(*x) |= rawbyte() & 0xff;
}

//take the contents from another unmarshall object
void
unmarshall::take_in(unmarshall &another)
{
	if (_buf)
		free(_buf);
	another.take_buf(&_buf, &_sz);
	_ind = RPC_HEADER_SZ;
	_ok = _sz >= RPC_HEADER_SZ?true:false;
}

bool
unmarshall::okdone()
{
	if(ok() && _ind == _sz) {
		return true;
	} else {
		return false;
	}
}

unsigned int
unmarshall::rawbyte()
{
	char c = 0;
	if (_ind >= _sz)
		_ok = false;
	else
		c = _buf[_ind++];
	return c;
}

unmarshall &
operator>>(unmarshall &u, unsigned char &x)
{
	x = (unsigned char) u.rawbyte() ;
	return u;
}

unmarshall &
operator>>(unmarshall &u, char &x)
{
	x = (char) u.rawbyte();
	return u;
}


unmarshall &
operator>>(unmarshall &u, unsigned short &x)
{
	x = (u.rawbyte() & 0xff) << 8;
	x |= u.rawbyte() & 0xff;
	return u;
}

unmarshall &
operator>>(unmarshall &u, short &x)
{
	x = (u.rawbyte() & 0xff) << 8;
	x |= u.rawbyte() & 0xff;
	return u;
}

unmarshall &
operator>>(unmarshall &u, unsigned int &x)
{
	x = (u.rawbyte() & 0xff) << 24;
	x |= (u.rawbyte() & 0xff) << 16;
	x |= (u.rawbyte() & 0xff) << 8;
	x |= u.rawbyte() & 0xff;
	return u;
}

unmarshall &
operator>>(unmarshall &u, int &x)
{
	x = (u.rawbyte() & 0xff) << 24;
	x |= (u.rawbyte() & 0xff) << 16;
	x |= (u.rawbyte() & 0xff) << 8;
	x |= u.rawbyte() & 0xff;
	return u;
}

unmarshall &
operator>>(unmarshall &u, unsigned long long &x)
{
	unsigned int h, l;
	u >> h;
	u >> l;
	x = l | ((unsigned long long) h << 32);
	return u;
}

unmarshall &
operator>>(unmarshall &u, std::string &s)
{
	unsigned sz;
	u >> sz;
	if(u.ok())
		u.rawbytes(s, sz);
	return u;
}

void
unmarshall::rawbytes(std::string &ss, unsigned int n)
{
	if ((_ind+n) > (unsigned)_sz) {
		_ok = false;
	} else {
		std::string tmps = std::string(_buf+_ind, n);
		swap(ss, tmps);
		assert(ss.size() == n);
		_ind += n;
	}
}




///FOLLOWING METHOD NOT USED YET -- FIXME
int
cmp_timespec(const struct timespec &a, const struct timespec &b)
{
	if (a.tv_sec > b.tv_sec)
		return 1;
	else if (a.tv_sec < b.tv_sec)
		return -1;
	else {
		if (a.tv_nsec > b.tv_nsec)
			return 1;
		else if (a.tv_nsec < b.tv_nsec)
			return -1;
		else
			return 0;
	}
}

void
add_timespec(const struct timespec &a, int b, struct timespec *result)
{
	// convert to millisec, add timeout, convert back
	result->tv_sec = a.tv_sec + b/1000;
	result->tv_nsec = a.tv_nsec + (b % 1000) * 1000000;
	assert(result->tv_nsec >= 0);
	while (result->tv_nsec > 1000000000) {
		result->tv_sec++;
		result->tv_nsec-=1000000000;
	}
}

int
diff_timespec(const struct timespec &end, const struct timespec &start)
{
	int diff = (end.tv_sec > start.tv_sec)?(end.tv_sec-start.tv_sec)*1000:0;
	assert(diff || end.tv_sec == start.tv_sec);
	if (end.tv_nsec > start.tv_nsec) {
		diff += (end.tv_nsec-start.tv_nsec)/1000000;
	} else {
		diff -= (start.tv_nsec-end.tv_nsec)/1000000;
	}
	return diff;
}


//FAST RPC server methods
void rpcs::handle_comm_q(server_lock_t* rpc_serv) {

  unsigned now_serv/* , timeout = 100 */;
  now_serv = rpc_serv->now_serving_rpc_comm;
  //  while((timeout) && (rpc_serv->rpc_comm_l->ticket == now_serv)) timeout--;
  
  //  if(timeout == 0)
  //return;

  if((rpc_serv->rpc_comm_l->ticket) == now_serv)
    return;

  if( (now_serv == MAX_TICKETS-1) && (rpc_serv->rpc_comm_l->ticket) > now_serv) { //boundary check!
    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "s: curr_ticket %d\n", rpc_serv->rpc_comm_l->ticket);
    return;
  }    
  
  DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Got someone in comm_queue... nowserv: %d\n", now_serv);

  //someone in queue, serve and increment now_serving_rpc_comm + 1 and num_expected ++;
  //this assumes that there wont be more than MAX_TICKETS set of processes/threads competing
  
  //+ 1 for ignoring the ticket itself
  if(rpc_serv->rpc_comm_sig[now_serv].signal != 0) { //client is not already signaled
    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "signal: %d, for nowserv: %d\n", rpc_serv->rpc_comm_sig[now_serv].signal, now_serv);
    return; //lets come back later
    //assert(0 && "signal not reset or free");
  }
  else if(rpc_serv->head_send_q[now_serv].signal != 0) {
    DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "signal: %d, for nowserv: %d is not released yet\n", rpc_serv->rpc_comm_sig[now_serv].signal, now_serv);

    return; //lets come back later
  }

  rpc_serv->rpc_comm_sig[now_serv].offset = now_serv; 
//&(rpc_serv->head_send_q[now_serv]) - &(rpc_serv->rpc_comm_l); //the offset to the data section


  send_queue_t* temp = (send_queue_t*) malloc(sizeof(send_queue_t));
  assert(clock_gettime(CLOCK_REALTIME, &(temp->start)) ==0); //record the start time
  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &(temp->start));
  //should not be in use! (in use == 1|2)

  __sync_synchronize();

  //now signal client
  rpc_serv->rpc_comm_sig[now_serv].signal = 1;

  temp->rpc_request = &(rpc_serv->head_send_q[now_serv]);
  //FIXME: should be used for timeout
  temp->tstamp = rpc_serv->gtstamp; //mark the current timestamp
  temp->client_ticket = now_serv; 
  rpc_serv->sendq.push_back(temp);

  if(rpc_serv->num_expected - rpc_serv->num_served >= MAX_TICKETS - 1)
    assert( 0 && "full!!!");

  ++(rpc_serv->now_serving_rpc_comm);
  (rpc_serv->now_serving_rpc_comm) %= MAX_TICKETS;
  ++(rpc_serv->num_expected);

}

void rpcs::mt_log_service(unsigned id, timespec start) {
  timespec endtime, tdiff;
  char tmp[128];
  unsigned len = rpc_log.len[id] + MAX_LOG_FILE_SIZE_PT*id;
  assert(clock_gettime(CLOCK_REALTIME, &endtime) == 0);
  //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endtime);
  rpc_reg.tot_served[id]++;

  /* sprintf(&tmp[0], "%d: %ld %ld %ld # %ld %ld %ld\n",  */
  /* 	  id, endtime.tv_sec, endtime.tv_nsec, rpc_reg.tot_served[id], */
  /* 	  rpc_log.num_cycles[id], rpc_log.num_locked[id], rpc_log.spl_num_locked[id]); */
  tdiff =  diff(start,endtime);
  sprintf(&tmp[0], "#%d: %ld\n", 
	  id,tdiff.tv_sec *1000000000 + tdiff.tv_nsec/* , rpc_reg.tot_served[id] */);

  sprintf(&(rpc_log.file[len]), "%s",tmp);
  rpc_log.len[id] += strlen(tmp); 
  if(rpc_log.len[id] >= MAX_LOG_FILE_SIZE_PT) {
    printf("%d: LOG FILE FULL!!! exiting..\n", id);
    exit(0);
  }

}


void rpcs::handle_send_q(server_lock_t* rpc_serv, unsigned tid) {

  if(rpc_serv->num_expected == rpc_serv->num_served || rpc_serv->sendq.size() == 0)
    return;


  vector<send_queue_t*>::iterator curr;
  for(curr = (rpc_serv->sendq.begin()); curr != rpc_serv->sendq.end() ; curr++) {
    send_queue_t* currqe = *curr;
    //rpc_resp_t response;
    char resp[MAX_BUFF_SIZE];
    int ret, remove = 0, respSize;

    if(currqe->rpc_request->signal != 1) {
      //TODO: make sure if tstamp is what u think it is
/*       if( MAX_TIME  < ABS(rpc_serv->gtstamp - currqe->tstamp) ) { */
/* 	//your taking too much time! */
/* 	//erase from rpc_serv->sendq */
/* #ifdef DEBUG */
/* 	printf("Got removed!!! WTF!! x-( \n"); */
/* #endif */
/* 	// */
/* 	//remove = 1; */
/* 	//ret = -2; */
/* 	//assert( 0 && "NYI"); */
/* 	continue; */
/*       } */
/*       else */
      continue; //not yet signalled by client
    }

    //check checksum
    if(chkChksum(currqe->rpc_request->data, currqe->rpc_request->sizeInChar, currqe->rpc_request->checksum)) {
      ret = 10;
#ifdef DEBUG
      printf("Got a valid send req: %s..\n", currqe->rpc_request->data);
#endif
    }
    else {
#ifdef DEBUG
      printf("error with checksum\n");
#endif
	ret = -1; //remove
    }

#ifdef DEBUG
    printf("Server: Serving... %u\n", currqe->client_ticket);
#endif


    dispatch(&currqe->rpc_request->data[0], currqe->rpc_request->sizeInChar);

    //memcpy(currqe->rpc_request->data, &(currqe->client_ticket), sizeof(int));

    currqe->rpc_request->checksum = calcChksum(currqe->rpc_request->data, currqe->rpc_request->sizeInChar);
    /* currqe->rpc_request->sizeInChar = sizeof(int); */

    __sync_synchronize(); //memory barrier


    //FIXME: Relying on client to set the signal back to zero to make the block free
    currqe->rpc_request->signal = 2; //response complete
    mt_log_service(tid, currqe->start); //log the end time and total serviced in logfile

#ifdef DEBUG
    printf("Server: Signaled with response : %u..\n",  currqe->client_ticket);
#endif

    free(*curr); //deallocte it 
    rpc_serv->sendq.erase(curr); //remove it from queue
    curr--; //to be back at the new next node after the remove
      
    rpc_serv->num_served ++;
    if(rpc_serv->num_served == rpc_serv->num_expected) {
      rpc_serv->num_served = rpc_serv->num_expected = 0;
      rpc_serv->sendq.clear(); //safe???
#ifdef DEBUG
      printf("no more in queue\n");
#endif      
      break;
    }

  }

}


void* rpcs::map_shared_file_server(string filename, int * fdret, int type) {
  void* channel;
  struct stat statbuff;
  unsigned filesize;
  int fd = open(&filename[0], OPEN_MODE, PERM);
  if(fd < 0) {
    printf("ERROR: %d : opening file\n", fd);
    return NULL;
  }

  //resize file if it is not the one used for rpc_register or logfile
  if(type == BINDER) 
    filesize = MAX_SH_FILE_SIZE;
  else if(type == S_LOG_FILE)
    filesize = MAX_LOG_FILE_SIZE;
  else
    filesize = getMaxChannelSize();

  /* if it is too small, we will extend the file for the mmap */
  if(fstat(fd, &statbuff) != 0) {
    printf("ERROR: %d : failed to fstat file\n", fd);
    return NULL;
  }
  if(statbuff.st_size < filesize) {
    if(lseek(fd, filesize-1, SEEK_SET) == (off_t)-1) {
      printf("ERROR: %d : failed to lseek file\n", fd);
      return NULL;
    }
    if(write(fd, "\0", 1) < 0) {
      printf("ERROR: %d : failed to write to file\n", fd);
      return NULL;
    }
  }
    

  channel =  mmap (0, filesize, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, 0);

  if(channel == MAP_FAILED) {
    printf("ERROR mmaping file\n");
    return NULL;
  }

  assert(channel != NULL);
  if(fdret != NULL)
    *fdret = fd;
  return channel;

}

void rpcs::sanity_check() {
  rpc_msg_t msg;
  rpc_resp_t resp;
  rpc_sync_t rpc_lock;
  rpc_signal_wait_t rpc_sig;

  //FIXME: uncomment and assure they are same
  assert(sizeof(rpc_lock) == sizeof(rpc_sig));
  assert(sizeof(msg.data) == sizeof(resp)); //should be the same!
}

void rpcs::init_rpc_registry(const char* sh_file) {
  unsigned i;

  DBG_LOG(DBG_INFO, DBG_MODULE(rpc), "Init RPC registry %s\n", sh_file);

  srand(time(NULL));
  rpc_reg.rpc_reg_lock = (rpc_signal_wait_t*) map_shared_file_server(sh_file, NULL, BINDER);
  assert(rpc_reg.rpc_reg_lock != NULL);
  rpc_reg.rpc_reg_reply = (rpc_msg_t*) &(rpc_reg.rpc_reg_lock[1]);
  rpc_reg.rpc_queue.clear();
  for(i = 0; i < NUMTHREADS; i++) {
    rpc_reg.tot_served[i] = 0;
    rpc_log.len[i] = 0;
    rpc_log.num_cycles[i] = 0;
    rpc_log.spl_num_locked[i] = 0;
    rpc_log.num_locked[i] = 0;
  }
  rpc_log.file = (char*) map_shared_file_server(LOG_FILE, NULL, S_LOG_FILE);

  memset(rpc_reg.rpc_reg_lock, 0, sizeof(rpc_signal_wait_t) + sizeof(rpc_msg_t));
  memset(rpc_log.file, 0, MAX_LOG_FILE_SIZE);

}

void rpcs::init_rpc(char* filename, server_lock_t* ne) {
  ne->rpc_comm_l = (rpc_sync_t*) map_shared_file_server(filename, &(ne->fd));
  ne->rpc_comm_sig = (rpc_signal_wait_t*) &(ne->rpc_comm_l[1]);

  ne->head_send_q = getQHead(ne->rpc_comm_l); //heigher priority queue
  ne->now_serving_rpc_comm = 0;
  ne->num_expected = 0;
  ne->num_served = 0;
  ne->gtstamp = 0;
  ne->sendq.clear();

#ifdef RPCS_VER_1
  ne->lock_queue = UNLOCKED;
#endif

  memset(ne->rpc_comm_l, 0, sizeof(rpc_sync_t) + sizeof(rpc_signal_wait_t) * (MAX_TICKETS + 1));
  memset(ne->head_send_q, 0, sizeof(rpc_msg_t) * MAX_TICKETS);

}


void rpcs::check_registry_incoming() {
  server_lock_t* new_server_qe;

  if(rpc_reg.rpc_reg_lock->signal == 0) //unlocked state
    return;

  //locked, so someone is trying to register
  switch(rpc_reg.rpc_reg_reply->signal) {
  case 0: {
    if(rpc_reg.rpc_reg_lock->signal == 2) {
      //unbind request - client yet to give its id
      return;
    }

    char filename[24];
    new_server_qe = (server_lock_t*) malloc(sizeof(server_lock_t));
    new_server_qe->client_id = rand(); 
    sprintf(filename, "%d", new_server_qe->client_id);
    strcpy(rpc_reg.rpc_reg_reply->data, filename);
    init_rpc(filename, new_server_qe);
    rpc_reg.rpc_queue.push_back(new_server_qe);
    rpc_reg.rpc_reg_reply->signal = 1; // replied
    return;
  }
  case 1: {

    if(rpc_reg.rpc_reg_lock->signal == 1)
      return;//not yet read the reply?

    assert(rpc_reg.rpc_reg_lock->signal == 2 ); //unbind req 

    //unbind the client
    unsigned int cid;
    vector<server_lock_t*>::iterator itr;

    sscanf(rpc_reg.rpc_reg_reply->data,"%d", &cid);
    
    for(itr = rpc_reg.rpc_queue.begin(); itr != rpc_reg.rpc_queue.end(); itr++) {
      if((*itr)->client_id == cid)
	break;
    }

    if(itr == rpc_reg.rpc_queue.end())
      assert(0 && "client id for unbind NOT found!");
    close((*itr)->fd);
    rpc_reg.rpc_queue.erase(itr);
#ifdef DEBUG
    cout << "Client unbound!\n";
#endif
    //reset the lock and signal
    rpc_reg.rpc_reg_reply->signal = 0; //reset the signal
    rpc_reg.rpc_reg_lock->signal = 0;  //reset lock
    return;
  }
  case 2: //client acked back
    assert(rpc_reg.rpc_reg_lock->signal ==1); //unbinded req shouldnt reach this point
    rpc_reg.rpc_reg_reply->signal = 0; //reset the signal
    rpc_reg.rpc_reg_lock->signal = 0;  //reset lock
    return;
  default:
    assert( 0 && "Unreachable");
  }
}


void* rpcs::rpc_server_kernel(void* arg) {
  vector<server_lock_t*>::iterator curr;
  server_lock_t* s_ce;
  sthr_args_t *targ = (sthr_args_t*) arg;
  unsigned tid = targ->tid;
  unsigned len = MAX_LOG_FILE_SIZE_PT*tid ; //+ 0 -- it is the start of the log area
  char tmp[32];

  assert(pin_to_core(targ->core) == 0);

  //HARIS: release the caller
  pthread_mutex_unlock(&targ->mutex);
  /* sprintf(&tmp[0], "%d pinned-to %d \n", tid, targ->core ); */
  /* sprintf(&(rpc_log.file[len]), "%s",tmp); */
  /* rpc_log.len[tid] = strlen(tmp);  */
  rpc_log.len[tid] = 0;

  while(1) {

#ifdef RPCS_VER_2
    int myturn;
#endif

    //DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Check Registry\n");

    if(tid == 0) //dispatched thread
      check_registry_incoming();

#ifdef DEBUG 
    //printf("S: Finished checking registry ..\n");
#endif

#ifdef RPCS_VER_2
    //goes to the starting "client queue" based on thread id
    //hence later on the round robin is take care of
    for(myturn = 0, curr = rpc_reg.rpc_queue.begin();
 	curr != rpc_reg.rpc_queue.end();
	curr++, myturn++) {
   
      if( myturn % NUMTHREADS != tid )
	continue;
      
      s_ce = *curr;

#ifdef DEBUG 
      //printf("S: Going into handling COMM requests ..\n");
#endif

      //services only one of them and then returns
      handle_comm_q(s_ce); 

#ifdef DEBUG 
      //  printf("S: Going into handling SEND requests ..\n");
#endif

      //higher priority -- checks for all of them at once
      handle_send_q(s_ce, tid); 

      //increment the globaltimestamp for each service-loop
      s_ce->gtstamp++; 

#ifdef DEBUG 
      //  printf("S: DONE with one SERVICE LOOP..\n");
#endif
    }
 

#else //RPCS_VER_1

    for(curr = rpc_reg.rpc_queue.begin();
 	curr != rpc_reg.rpc_queue.end();
	curr++) {
      s_ce = *curr;
      if(s_ce->lock_queue == 0) {
	//try to lock it
	if(! __sync_bool_compare_and_swap(&(s_ce->lock_queue), UNLOCKED, LOCKED)) {
	  rpc_log.spl_num_locked[tid]++;
	  continue;
	}
	// at this point you got the lock
	assert(s_ce->lock_queue == 1);
	//proceed to process the queue
      }
      else {
	rpc_log.num_locked[tid]++;
	continue; //locked look for some other work
      }
#ifdef DEBUG 
      //printf("S: Going into handling COMM requests ..\n");
#endif

      //services only one of them and then returns
      handle_comm_q(s_ce); 

#ifdef DEBUG 
      //  printf("S: Going into handling SEND requests ..\n");
#endif

      //higher priority -- checks for all of them at once
      handle_send_q(s_ce, tid); 

      //increment the globaltimestamp for each service-loop
      s_ce->gtstamp++; 

#ifdef DEBUG 
      //  printf("S: DONE with one SERVICE LOOP..\n");
#endif
      s_ce->lock_queue = UNLOCKED; //unlock queue since you are done with one service cycle
    }
 
#endif
    rpc_log.num_cycles[tid]++;

  }//service forever!


}


static void* server_kernel_handler(void* arg) {
  rpcs *serv = ((sthr_args*) arg)->serv_obj;
  serv->rpc_server_kernel(arg);
}

int rpcs::main_service_loop()
{
	return main_service_loop(bind_f_.c_str());
}


int rpcs::main_service_loop(const char* sh_file) {

  assert(NUMTHREADS == 1); //only single thread for now!

  sanity_check();
  init_rpc_registry(sh_file);

  int rc;
  unsigned tid;
  pthread_t thread[NUMTHREADS];
  sthr_args_t t_args[NUMTHREADS];


  DBG_LOG(DBG_DEBUG, DBG_MODULE(rpc), "Fork service threads\n");
  for(tid = 0; tid < NUMTHREADS; tid++) {

    t_args[tid].tid = tid;
    t_args[tid].core = tid;//atoi(argv[tid+1]);
    t_args[tid].serv_obj = this;
	pthread_mutex_init(&t_args[tid].mutex, NULL);
	pthread_mutex_lock(&t_args[tid].mutex);
    if( (rc=pthread_create( &thread[tid], NULL, server_kernel_handler, &t_args[tid])) )  {
      printf("Thread creation failed: %d\n", rc);
    }
	pthread_mutex_lock(&t_args[tid].mutex); // wait for the forked thread
  }
//  for(tid = 0; tid <NUMTHREADS; tid++) {
 //   pthread_join( thread[tid], NULL);
  //}


  return 0;
}
