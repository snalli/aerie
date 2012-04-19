
#include <stdlib.h>
#include <time.h>

#define S_LOG_FILE 10

namespace rpcfast {

class rpcs;

typedef struct sthr_args {
  unsigned tid, core;
  rpcs* serv_obj;
  pthread_mutex_t mutex;
} sthr_args_t;


// each server thread goes over the queue of client queues
//#define RPCS_VER_1

// each server thread handles a set of clients assigned in RR across server threads
//#define RPCS_VER_2

#define TIMEOUT 3000

//FIXME: need to guess a better value?
#define MAX_TIME 5000

//local to the server, will be changed ONLY by server 
//
} // namespace rpcfast
