/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#ifndef _KEYVALUESTORE_TOKYOCABINETADAPTER_H
#define _KEYVALUESTORE_TOKYOCABINETADAPTER_H

#include <tcutil.h>
#include <tchdb.h>
#include "../../syncKeyValueStore.h"

namespace keyValueStore
{
namespace adapters
{
namespace tokyoCabinetAdapter
{

/**
 * This class implements adapter for tokyo cabinet
 */
class TokyoCabinetAdapter : public SyncKeyValueStore
{
public:
    TokyoCabinetAdapter(SyncKeyValueStoreConfigs const& config);

    virtual ~TokyoCabinetAdapter();

    virtual void processRequests(char* buffer,
            SafeKVStoreQueue& requestQueue,
            boost::mutex::scoped_lock& queueLock);


    virtual void commit();

    virtual void reset();

private:

    TCHDB* db;

    volatile uint64 numInserts;
    volatile uint64 totalInsertLatency;
    volatile uint64 numLookups;
    volatile uint64 totalLookupLatency;
    volatile uint64 numDeletes;
    volatile uint64 totalDeleteLatency;

    void initializeDb(string const& devicePath, SyncKeyValueStoreConfigs::Mode mode);
    
    void insert(InsertRequest& r, char* buffer);
   
    void remove(RemoveRequest& r, char* buffer);

    void lookup(LookupRequest& r, char* buffer);
};

}
}
}

#endif
