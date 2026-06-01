/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#include <keyValueStore/kvStoreQueue.h>

using namespace keyValueStore;

SafeKVStoreQueue::SafeKVStoreQueue()
{
}

SafeKVStoreQueue::~SafeKVStoreQueue()
{
    //DASSERT( this->queue.empty(), "outstanding requests");
}

AbstractKVStoreRequest* SafeKVStoreQueue::pop()
{
    if( this->queue.empty())
    {
        return 0;
    }
    AbstractKVStoreRequest* req = this->queue.front();
    this->queue.pop_front();
    return req;
}

// This function is called with the lock already held
void SafeKVStoreQueue::push_front( AbstractKVStoreRequest* req)
{
    bool empty = this->queue.empty();
    this->queue.push_front( req);
    if( empty) 
    {
        this->waitCondition.notify_one();
    }
}

void SafeKVStoreQueue::push_back( AbstractKVStoreRequest* req)
{
    bool empty = this->queue.empty();
    this->queue.push_back( req);
    if( empty) 
    {
        this->waitCondition.notify_one();
    }
}

void SafeKVStoreQueue::safe_push_back( AbstractKVStoreRequest* req)
{
    boost::mutex::scoped_lock lock( this->mutex);
    bool empty = this->queue.empty();
    this->queue.push_back( req);
    if( empty) 
    {
        this->waitCondition.notify_one();
    }

}

bool SafeKVStoreQueue::empty()
{
    return this->queue.empty();
}

uint32_t SafeKVStoreQueue::size()
{
    return this->queue.size();
}
