/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#include <tcutil.h>
#include "tokyoCabinetAdapter.h"
#include <util/boostTime.h>

using namespace util;
using namespace keyValueStore::adapters::tokyoCabinetAdapter;

TokyoCabinetAdapter::TokyoCabinetAdapter(SyncKeyValueStoreConfigs const& config)
    : SyncKeyValueStore(config),
      numInserts(0), totalInsertLatency(0),
      numLookups(0), totalLookupLatency(0),
      numDeletes(0), totalDeleteLatency(0)
{
    this->initializeDb(config.getDevicePath(), config.getMode());
}

static string printError(int err)
{
    return string(tchdberrmsg(err));
}

void TokyoCabinetAdapter::initializeDb(string const& devicePath, SyncKeyValueStoreConfigs::Mode mode)
{
    this->db = tchdbnew();

    if (mode == SyncKeyValueStoreConfigs::NORMAL)
    {
        bool ret = tchdbtune(this->db, -1, -1, -1, HDBTLARGE);
        if (!ret)
        {
            int err = tchdbecode(this->db);
            switch (err)
            {
                case TCEINVALID:
                    THROW(ExceptionInvalidArgument, "tchdbtune failed " << printError(err));
                    break;
                default:
                    THROW(MiscException, "tchdbtune failed " << printError(err));
            }
        }

        ret = tchdbsetmutex(this->db);
        if (!ret)
        {
            int err = tchdbecode(this->db);
            THROW(MiscException, "tchdbtune failed " << printError(err));
        }

        ret = tchdbopen(this->db, devicePath.c_str(), HDBOWRITER | HDBOREADER | HDBOCREAT| HDBOTRUNC | HDBOTSYNC);
        if (!ret)
        {
            int err = tchdbecode(this->db);
            THROW(ExceptionDeviceFailure, "tchdbopen failed " << printError(err));
        }
    }
    else
    {
        bool ret = tchdbopen(this->db, devicePath.c_str(), HDBOREADER);
        if (!ret)
        {
            int err = tchdbecode(this->db);
            THROW(ExceptionDeviceFailure, "tchdbopen failed " << printError(err));
        }
    }
}

TokyoCabinetAdapter::~TokyoCabinetAdapter()
{
    bool ret = tchdbclose(this->db);
    if (!ret)
    {
        int err = tchdbecode(this->db);
        THROW(ExceptionDeviceFailure, "tchdbclose failed " << printError(err));
    }
}

void TokyoCabinetAdapter::processRequests(char* buffer, SafeKVStoreQueue& requestQueue, boost::mutex::scoped_lock& queueLock)
{
    DLOG(1, "TokyoCabinetAdapter processRequests");
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
                DLOG(1, "TokyoCabinetAdapter InsertRequest");
                InsertRequest& r = dynamic_cast<InsertRequest&>(*req);
                this->insert(r, buffer);
            }
            break;
        case REMOVE:
            {
                DLOG(1, "TokyoCabinetAdapter RemoveRequest");
                RemoveRequest& r = dynamic_cast<RemoveRequest&>(*req);
                this->remove(r, buffer);
            }
            break;
        case LOOKUP:
            {
                DLOG(1, "TokyoCabinetAdapter LookupRequest");
                LookupRequest& r = dynamic_cast<LookupRequest&>(*req);
                this->lookup(r, buffer);
            }
            break;
        default:
            FAILURE("unknown request type");
            break;
    }

    delete req;
}

void TokyoCabinetAdapter::insert(InsertRequest& r, char* buffer)
{
    ++this->numInserts;
    uint64 startTime = util::FastTime::getMicroseconds();
    uint32 keySize = r.getKey().getSerializedSize(); 
    uint32 valSize = r.getValue().getSerializedSize();

    char* keyBuf = buffer;
    char* valBuf = buffer + keySize;

    r.getKey().serialize(keyBuf);

    r.getValue().serialize(valBuf);

    bool ret = tchdbput(this->db, keyBuf, keySize, valBuf, valSize);
    this->totalInsertLatency += util::FastTime::getMicroseconds() - startTime;
    if (!ret)
    {
        int err = tchdbecode(this->db);
        r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionDeviceFailure, "tchdbput failed " << printError(err))));
    }
    else
    {
        r.getSuccessCallback()(&(r.getKey()));
    }

}

void TokyoCabinetAdapter::commit()
{
}

void TokyoCabinetAdapter::lookup(LookupRequest& r, char* buffer)
{
    ++this->numLookups;
    uint64 startTime = util::FastTime::getMicroseconds();
    uint32 keySize = r.getKey().getSerializedSize();

    char* keyBuf = buffer;
    char* valBuf = buffer + keySize;
   
    r.getKey().serialize(keyBuf);

    //XXX hack
    uint32 valuePtrLen = 512 * 1024;
    int size = tchdbget3(this->db, keyBuf, keySize, valBuf, valuePtrLen);
    if (size == -1)
    {
        this->totalLookupLatency += util::FastTime::getMicroseconds() - startTime;
        r.getSuccessCallback()(&(r.getKey()), NOT_FOUND);
    }
    else
    {
        r.getValue().deserialize(valBuf);
        this->totalLookupLatency += util::FastTime::getMicroseconds() - startTime;
        r.getSuccessCallback()(&(r.getKey()), FOUND);
    }
}

void TokyoCabinetAdapter::remove(RemoveRequest& r, char* buffer)
{
    ++this->numDeletes;
    uint64 startTime = util::FastTime::getMicroseconds();
    uint32 keySize = r.getKey().getSerializedSize(); 
    char* keyBuf = buffer;
   
    r.getKey().serialize(keyBuf);

    bool ret = tchdbout(this->db, keyBuf, keySize);
    this->totalDeleteLatency += util::FastTime::getMicroseconds() - startTime;
    if (!ret)
    {
        int err = tchdbecode(this->db);
        switch (err)
        {
            case TCENOREC:
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionValueNonExistent, printError(err))));
                break;
            default:
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionDeviceFailure, "tchdbout failed " << printError(err))));
        }

    }
    else
    {
        r.getSuccessCallback()(&(r.getKey()));
    }
}

void TokyoCabinetAdapter::reset()
{
    bool ret = tchdbvanish(this->db);
    if (!ret)
    {
        int err = tchdbecode(this->db);
        switch (err)
        {
            case TCEINVALID:
                //THROW(ExceptionInvalidArgument, "tchdbtune failed " << printError(err));
                FAILURE("tchdbvanish failed " << printError(err));
                break;
            default:
                //THROW(MiscException, "tchdbtune failed " << printError(err));
                FAILURE("tchdbvanish failed " << printError(err));
        }
    }
}
