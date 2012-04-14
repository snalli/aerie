// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"

#include <sstream>
#include <iostream>
#include <stdio.h>

lock_client::lock_client(char* dst)
{
  cl = new rpcc(dst);
  if (cl->bind() < 0) {
    cout << "lock_client: call bind failed\n";
  }
}

lock_client::~lock_client()
{
  if(cl->unbind() < 0)
    cerr << "lock_client: call unbind failed\n";
  // else
  //   printf("Client unbound successful!\n");
  
}

int
lock_client::stat(lock_protocol::lockid_t lid)
{
  int r;
  int ret = cl->call(lock_protocol::stat, cl->id(), lid, r);
  assert (ret == lock_protocol::OK);
  return r;
}

lock_protocol::status
lock_client::acquire(lock_protocol::lockid_t lid)
{
}

lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid)
{
}

