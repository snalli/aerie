/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#include <iomanip>
#include "memMap.h"

#undef MEMMAP_DEBUG

using namespace util;
using namespace keyValueStore::adapters::memMapAdapter;

static boost::mutex mem_mutex;

MemKey::MemKey(uint len, void* buf) : len(len)
{
    this->buf = new unsigned char[len];
    ::memcpy(this->buf, buf, len);
}

MemKey::MemKey(MemKey const& other)
{
    this->len = other.len;
    this->buf = new unsigned char[other.len];
    ::memcpy(this->buf, other.buf, other.len);
}

MemKey::~MemKey()
{
    delete [] this->buf;
}

bool MemKey::operator<(MemKey const& other) const
{
        uint len = std::min(this->len, other.len);
        int cmp = ::memcmp(this->buf, other.buf, len);
        if (cmp == 0)
        {
            return this->len < other.len;
        }
        return cmp < 0;
}

ostream& MemKey::print(ostream& str) const
{
    str << "MemKey: len " << this->len << ", buf ";
    for (uint i = 0; i < this->len; i++)
    {
        if (::isprint(this->buf[i]))
        {
            str << this->buf[i];
        }
        else
        {
            uint ch = this->buf[i];
            str << "\\" << oct << setw(3) << setfill('0') << ch << dec;
        }
    }
    return str;
}

MemValue::MemValue(uint len, void* buf) : len(len)
{
    this->buf = new unsigned char[len];
    ::memcpy(this->buf, buf, len);
}

MemValue::~MemValue()
{
    delete [] this->buf;
}

MemMapAdapter::MemMapAdapter(SyncKeyValueStoreConfigs const& config)
    : SyncKeyValueStore(config)
{
}

MemMapAdapter::~MemMapAdapter()
{
    map<MemKey, MemValue*>::iterator iter, curIter, end;
    end = this->inMemMap.end();
    iter = this->inMemMap.begin();
    while (iter != end)
    {
        curIter = iter++;
        delete curIter->second;
        this->inMemMap.erase(curIter);
    }
}

void MemMapAdapter::processRequests(char*, SafeKVStoreQueue& requestQueue, boost::mutex::scoped_lock& queueLock)
{
    DLOG(1, "MemMapAdapter processRequests");
    AbstractKVStoreRequest* req = requestQueue.pop();
    queueLock.unlock();
    if (req == NULL)
    {
        return;
    }

    switch (req->getType())
    {
        case INSERT:
            {
                DLOG(1, "MemMapAdapter InsertRequest");
                InsertRequest& r = dynamic_cast<InsertRequest&>(*req);
                this->insert(r);
            }
            break;
        case REMOVE:
            {
                DLOG(1, "MemMapAdapter RemoveRequest");
                RemoveRequest& r = dynamic_cast<RemoveRequest&>(*req);
                this->remove(r);
            }
            break;
        case LOOKUP:
            {
                DLOG(1, "MemMapAdapter LookupRequest");
                LookupRequest& r = dynamic_cast<LookupRequest&>(*req);
                this->lookup(r);
            }
            break;
        case RESET:
            {
                DLOG(1, "MemMapAdapter ResetRequest");
                ResetRequest& r = dynamic_cast<ResetRequest&>(*req);
                this->reset(r);
            }
            break;
        default:
            FAILURE("unknown request type");
            break;
    }
    delete req;
}

void MemMapAdapter::insert(InsertRequest& r)
{
    uint32 keySize = r.getKey().getSerializedSize();
    char keyBuf[keySize];
    r.getKey().serialize(keyBuf);
    
    uint32 valSize = r.getValue().getSerializedSize();
    char valBuf[valSize];
    r.getValue().serialize(valBuf);


    MemKey memKey(keySize, keyBuf);
    boost::mutex::scoped_lock debug_lock(mem_mutex);
//cerr << "MemMap insert: " << memKey << endl;
#ifdef MEMMAP_DEBUG
    cerr << "MemMap insert: " << memKey << endl;
#endif
    MemValue* memValue = new MemValue(valSize, valBuf);
    pair<map<MemKey, MemValue*>::iterator, bool> res = this->inMemMap.insert(make_pair(memKey, memValue));
    if (!res.second)
    {
#ifdef MEMMAP_DEBUG
        cerr << "MemMap: key already exists (replacing)" << endl;
#endif
	PASSERT(false, "shouldn't insert same key twice key: " << memKey);
    delete res.first->second;
    res.first->second = memValue;
    }
#ifdef MEMMAP_DEBUG
    map<MemKey, MemValue*>::iterator iter = this->inMemMap.begin();
    cerr << "Map Dump:" << endl;
    while (iter != this->inMemMap.end())
    {
        cerr << "MapKey " << iter->first << endl;
        iter++;
    }
#endif

    r.getSuccessCallback()(&(r.getKey()));
}

void MemMapAdapter::commit()
{
}

void MemMapAdapter::lookup(LookupRequest& r)
{
    uint32 keySize = r.getKey().getSerializedSize(); 
    char keyBuf[keySize];
    r.getKey().serialize(keyBuf);
    MemKey memKey(keySize, keyBuf);
    
    boost::mutex::scoped_lock debug_lock(mem_mutex);
#ifdef MEMMAP_DEBUG
    cerr << "MemMap lookup: " << memKey << endl;
#endif
    map<MemKey, MemValue*>::iterator iter = this->inMemMap.find(memKey);
    if (iter == this->inMemMap.end())
    {
#ifdef MEMMAP_DEBUG
        cerr << "MemMap key not found" << endl;
#endif
        r.getSuccessCallback()(&(r.getKey()), NOT_FOUND);
    }
    else
    {
#ifdef MEMMAP_DEBUG
        cerr << "MemMap key found" << endl;
#endif
        uint32 valSize = iter->second->len; 
        char valBuf[valSize];
        ::memcpy(valBuf, iter->second->buf, valSize);
        r.getValue().deserialize(valBuf);
        r.getSuccessCallback()(&(r.getKey()), FOUND);
    }
}

void MemMapAdapter::remove(RemoveRequest& r)
{
    uint32 keySize = r.getKey().getSerializedSize();
    char keyBuf[keySize];
    r.getKey().serialize(keyBuf);
    MemKey memKey(keySize, keyBuf);
    boost::mutex::scoped_lock debug_lock(mem_mutex);
#ifdef MEMMAP_DEBUG
    cerr << "MemMap remove: " << memKey << endl;
#endif
    map<MemKey, MemValue*>::iterator iter = this->inMemMap.find(memKey);
    if (iter == this->inMemMap.end())
    {
        r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionValueNonExistent, "key not found in MemMap")));
    }
    else
    {
        this->inMemMap.erase(iter);
        r.getSuccessCallback()(&(r.getKey()));
    }
}

void MemMapAdapter::reset(ResetRequest& r)
{
    this->inMemMap.clear();
    r.getSuccessCallback()();
}
