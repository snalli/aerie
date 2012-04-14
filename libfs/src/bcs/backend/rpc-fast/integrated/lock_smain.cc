#include "rpc.h"
#include <stdlib.h>
#include <stdio.h>
#include "lock_server.h"



int
main(int argc, char *argv[])
{

  if(argc != 2){
    fprintf(stderr, "Usage: %s shared_file_name\n", argv[0]);
    exit(1);
  }

  lock_server ls;
  rpcs server(argv[1]);
  server.reg(lock_protocol::stat, &ls, &lock_server::stat);

  server.main_service_loop(argv[1]);
}
