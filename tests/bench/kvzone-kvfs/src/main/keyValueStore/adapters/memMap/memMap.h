/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#ifndef _KEYVALUESTORE_MEMMAPADAPTER_H
#define _KEYVALUESTORE_MEMMAPADAPTER_H

#include <map>
#include <ostream>
#include "../../syncKeyValueStore.h"

namespace keyValueStore
{
namespace adapters
{
namespace memMapAdapter
{

class MemKey
{
public:
    MemKey(uint len, void* buf);
    MemKey(MemKey const& other);
    ~MemKey();
    ostream& print(ostream& str) const;
    bool operator<(MemKey const& other) const;

    uint len;
    unsigned char* buf;
};

inline ostream& operator<<(ostream& ostr, MemKey const& key)
{
    return key.print(ostr);
}

class MemValue
{
public:
    MemValue(uint len, void* buf);
    ~MemValue();

    uint len;
    unsigned char* buf;
};

/**
 * This class implements adapter for tokyo cabinet
 */
class MemMapAdapter : public SyncKeyValueStore
{
public:
    MemMapAdapter(SyncKeyValueStoreConfigs const& config);
    virtual ~MemMapAdapter();
    virtual void commit();
    virtual void processRequests(char*,
            SafeKVStoreQueue& requestQueue,
            boost::mutex::scoped_lock& queueLock);

private:
    map<MemKey, MemValue*> inMemMap;

    void insert(InsertRequest& r);
    void remove(RemoveRequest& r);
    void lookup(LookupRequest& r);
    void reset(ResetRequest& r);
};

}
}
}

#endif
