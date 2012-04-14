//
// Lock demo
//

#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "papi.h"

lock_client *lc;

//#define PERF

int
main(int argc, char *argv[])
{
  int r;//, count = 25;

  if(argc != 2){
    fprintf(stderr, "Usage: %s bind_file\n", argv[0]);
    exit(1);
  }

  lc = new lock_client(argv[1]);


  //  while(--count) {
  r = lc->stat(99);
  printf ("stat returned %d\n", r);
  // }
  
  delete lc;
}
