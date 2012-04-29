/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#include <iostream>
#include "workersQueue.h"
#include <boost/lexical_cast.hpp>

using namespace util;
using namespace keyValueStore;

WorkerPool::WorkerPool( std::string const & threadPrefix, 
        util::uint16 const numThreads,
        SyncKeyValueStore& syncKeyValueStore,
        SafeKVStoreQueue & requestQueue )
    : stillWorking(true)
    , threadCounter(0)
    , syncKeyValueStore(syncKeyValueStore)
    , requestQueue(requestQueue)
{
    PASSERT(numThreads > 0, "Number of threads must be greater than 0");
    this->threadCounter = numThreads;
    for (uint16 i = 0; i < numThreads; ++i)
    {
        std::string threadName = threadPrefix ;
        threadName += boost::lexical_cast<std::string>(i) ;

        WorkerThread * thread = new WorkerThread(threadName, *this);
        this->workerThreads.push_back(thread);
        thread->start();
    }
}

WorkerPool::~WorkerPool()
{
    DLOG(1, "WorkerPool stopping all workers");
    uint numActiveThreads;
    do
    {
        this->requestQueue.waitCondition.notify_all();

        boost::mutex::scoped_lock lock(this->requestQueue.mutex);
        this->stillWorking = false;
        numActiveThreads = this->threadCounter;
    } while( numActiveThreads > 0) ;

    DLOG(1, "WorkerPool stopped all workers");
    // workerThreads destructor will join all threads
    boost::ptr_list<WorkerThread>::iterator iter = this->workerThreads.begin();
    boost::ptr_list<WorkerThread>::iterator end = this->workerThreads.end();

    for (; iter != end; ++iter)
    {
        iter->join();
    }
}

//**************************************************************
//                     Worker
//*************************************************************

/** WorkerThread inside workersQueue::WorkerPool
 *
 */
WorkerPool::WorkerThread::WorkerThread(
        std::string const & name, 
        WorkerPool & pool )
    : pool(pool), name(name)
{
    int ret = posix_memalign((void**)&this->buffer, 512, BUFFER_SIZE);
    (void)ret;
    memset(this->buffer, 0, BUFFER_SIZE);
}

WorkerPool::WorkerThread::~WorkerThread()
{
    free(this->buffer);
    this->thrd->join();
}
void WorkerPool::WorkerThread::start()
{
	this->thrd = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&WorkerThread::run, this)));
}
void WorkerPool::WorkerThread::run()
{
    for (;;)
    {
        DLOG(1, "WorkerThread calling waitForWork ");
        boost::mutex::scoped_lock lck(this->pool.requestQueue.mutex);
        // check if queue empty before waiting
        bool empty = false;
        while( (empty = this->pool.requestQueue.empty()) && this->pool.stillWorking)
        {
            DLOG(1, "WorkerThread waiting ");
            this->pool.requestQueue.waitCondition.wait( lck);
        }

        DLOG(1, "WorkerThread woke up ");
        // shutdown if shutdown and queue empty
        if( ! empty)
        {
            DLOG(1, "WorkerThread invoking processRequests on syncKeyValueStore ");
            this->pool.syncKeyValueStore.processRequests(this->buffer, this->pool.requestQueue, lck);
        }
        else
        {
            DLOG(1, "WorkerThread shutting down ");
            --this->pool.threadCounter;
            return;
        }
    }
}
