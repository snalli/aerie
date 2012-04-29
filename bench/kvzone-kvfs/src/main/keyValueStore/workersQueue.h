/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#ifndef  _WORKERS_QUEUE_H
#define  _WORKERS_QUEUE_H

#include <boost/ptr_container/ptr_list.hpp>
#include <boost/bind.hpp>

#include "util/types.h"
#include "util/exceptions.h"
#include "util/misc.h"
#include "keyValueStore/keyValueStoreTypes.h"
#include "syncKeyValueStore.h"

namespace keyValueStore
{
  //int32 const WORKER_THREAD_STACK_SIZE = (1024 * 1024 ) ;

  /**
   * Worker threads pool associated with one queue. If there are multiple queues, each has its own worker set.
   * 
   * Each worker is statically assigned a request queue to work on. The queue contains a mutex to
   * synchronize producers and consumers, as well as a condition to allow workers to sleep on it.
   *
   * The worker will synchoronously use the synckeyvaluestore to process one or more requests from the queue.
   */
  class WorkerPool
  {
  public:
	WorkerPool(std::string const & threadNamePrefix, 
			   util::uint16 numThreads,
			   SyncKeyValueStore& syncKeyValueStore,
			   SafeKVStoreQueue & requestQueue);
	~WorkerPool(); // Tells all workers to stop working

  protected:
	/**
	 * Thread which processes requests stored in WorkerPool
	 */
	class WorkerThread : public boost::thread
	{
	public:
	  /// Each thread is using the pool's syncKeyValueStore and requestQueue
	  WorkerThread( std::string const & name, WorkerPool& pool );
	  ~WorkerThread();
	  virtual void start();

	protected:
	  virtual void run();

	private:
	  WorkerPool& pool; // Worker's pool
	  boost::shared_ptr<boost::thread> thrd;
	  std::string name;
	  char* buffer; // Each worker has a buffer used to serialize the request(s) before issuing I/O
	};

	friend class WorkerThread;

	// has to be first in destruction list
	boost::ptr_list<WorkerThread> workerThreads;

	volatile bool stillWorking;

	util::uint16 threadCounter;

	SyncKeyValueStore& syncKeyValueStore ; /// The synchronous key value store used by the workers

	SafeKVStoreQueue & requestQueue;
  };
}

#endif
