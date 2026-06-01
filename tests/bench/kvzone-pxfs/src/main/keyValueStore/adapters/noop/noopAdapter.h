/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#ifndef _KEYVALUESTORE_NOOPADAPTER_H
#define _KEYVALUESTORE_NOOPADAPTER_H

#include "../../syncKeyValueStore.h"

namespace keyValueStore
{
namespace adapters
{
namespace noopAdapter
{

class NoopAdapter : public SyncKeyValueStore
{
public:
    NoopAdapter(SyncKeyValueStoreConfigs const& config);

    virtual ~NoopAdapter();

  /// Called with the queue and the lock on it already taken (also passed as argument so that the queue can be unlocks while the IO is done)
    virtual void processRequests(char* buffer,
				 SafeKVStoreQueue & requestQueue, 
				 boost::mutex::scoped_lock &queueLock);

    virtual void commit();

private:
    void insert(InsertRequest& r);
    void remove(RemoveRequest& r);
    void lookup(LookupRequest& r);
    void reset(ResetRequest& r);
};

}
}
}

#endif
