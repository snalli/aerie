/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef _KEYVALUESTORE_KVSTOREQUEUE_H
#define _KEYVALUESTORE_KVSTOREQUEUE_H

#include <deque>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

using namespace std;

namespace keyValueStore
{
  enum RequestType
  {
	INSERT = 72,
	REMOVE,
	LOOKUP,
    RESET,
  };
  

  /// The abstract base class of requests that are put into the queue
  class AbstractKVStoreRequest
  {
  public:
	AbstractKVStoreRequest();
	virtual ~AbstractKVStoreRequest();
	
	virtual RequestType getType() const = 0;
  }; /// class AbstractKVStoreRequest
  
  
  /// Queue of abstract requests. 
  /// Contains a mutex protecting operations on the queue, as well as
  /// a condition variable used to wake up worker threads waiting for work.
  ///
  /// This class does no do any memory management of requests
  class SafeKVStoreQueue
  {
  public:
	SafeKVStoreQueue();

	~SafeKVStoreQueue();
	
	AbstractKVStoreRequest* pop();
	
	void push_front( AbstractKVStoreRequest* req);
    
	void push_back( AbstractKVStoreRequest* req);
	
	/// Takes the lock before doing the push
	void safe_push_back( AbstractKVStoreRequest* req);
	
	bool empty();
	uint32_t size();
	
	boost::mutex mutex; /// Protects insertion, removals from the queue
	boost::condition waitCondition; // Condition on which workers assigned to this queue wait

  private:
	deque<AbstractKVStoreRequest*> queue;
  }; // class SafeKVStoreQueue
} // namespace keyValueStore

#endif
