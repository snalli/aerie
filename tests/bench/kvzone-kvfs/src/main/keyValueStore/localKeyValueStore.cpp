/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#include <boost/bind.hpp>
#include <util/misc.h>
#include <keyValueStore/localKeyValueStore.h>
#include "workersQueue.h"
#include <keyValueStore/keyValueStoreTypes.h>
#include "adapters/memMap/memMap.h"
#include "adapters/noop/noopAdapter.h"
#include "adapters/tokyoCabinet/tokyoCabinetAdapter.h"
#include "adapters/berkeleyDB/cds/berkeleyDBAdapter.h"
#include "adapters/sqlite/sqliteAdapter.h"
#include "boost/lexical_cast.hpp"

using namespace keyValueStore;
using keyValueStore::adapters::sqliteAdapter::SqliteAdapter;
using keyValueStore::adapters::tokyoCabinetAdapter::TokyoCabinetAdapter;
using keyValueStore::adapters::memMapAdapter::MemMapAdapter;
using keyValueStore::adapters::noopAdapter::NoopAdapter;
using keyValueStore::adapters::berkeleyDBAdapter::BerkeleyDBAdapter ;
//using keyValueStore::adapters::berkeleyDBAdapter::BerkeleyDB2Adapter ;

/***************  		class SubmitSTatus def's		*********************/

SubmitStatus::SubmitStatus( SubmissionStatus submissionStatus)
	:	submissionStatus(submissionStatus)
{
} // SubmitStatus


SubmissionStatus SubmitStatus::getStatus() const 
{ 
	return this->submissionStatus ;
} // getStatus


/*************			class LocalKeyValueStoreConfigs	 def's 		*************/

/// keyCompare is an operator that can take two ComparableSerializables and compare them.
/// the application needs to implement it and register it here if it wants the
/// keyvalue store to use  this operator instead of the default string compare
/// for indexing purposes.
LocalKeyValueStoreConfigs::LocalKeyValueStoreConfigs( 
	std::string const & devicePath,
	uint16 numThreads,
    uint64 storeSize,
    KeyValueStoreType type,
	KeyCompare *keyCompare 
	)
:	devicePath(devicePath)
,	numThreads(numThreads)
,   type(type)
,	keyCompare(keyCompare)
,	storeSize(storeSize)
{}


std::string const & LocalKeyValueStoreConfigs::getDevicePath() const 
{ 
	return this->devicePath ; 
}

uint16 LocalKeyValueStoreConfigs::getNumThreads() const 
{ 
	return this->numThreads; 
}

KeyCompare * LocalKeyValueStoreConfigs::getKeyCompare() const 
{ 
	return this->keyCompare; 
}

uint64 LocalKeyValueStoreConfigs::getStoreSize() const 
{ 
	return this->storeSize ; 
}

KeyValueStoreType LocalKeyValueStoreConfigs::getType() const
{
    return this->type;
}


/*****************		class LocalKeyValueStore defs			****************/

/**
  * constructor
  * @param		LocalKeyValueStoreConfigs
  *
  * comments	remainingStoreMemory maintains the amount of memory that is
  *				still available; be sure to operate on it whenever an
  *				insert or erase is processed.
  */
LocalKeyValueStore::LocalKeyValueStore( 
		LocalKeyValueStoreConfigs const& localKeyValueStoreConfigs) 
:    numQueues( localKeyValueStoreConfigs.getNumThreads()) // TODO correct misnomer
,	remainingStoreMemory( localKeyValueStoreConfigs.getStoreSize() )
{
    try
    {
        switch (localKeyValueStoreConfigs.getType())
        {
            case SQLITE :
                this->syncKeyValueStore =  new SqliteAdapter(
                        SyncKeyValueStoreConfigs(
                            localKeyValueStoreConfigs.getDevicePath(),
                            localKeyValueStoreConfigs.getStoreSize(),
                            localKeyValueStoreConfigs.getKeyCompare(),
                            SyncKeyValueStoreConfigs::NORMAL
                            ));
                break;
            case BERKELEYDB :
                this->syncKeyValueStore =  new BerkeleyDBAdapter(
                        SyncKeyValueStoreConfigs(
                            localKeyValueStoreConfigs.getDevicePath(),
                            localKeyValueStoreConfigs.getStoreSize(),
                            localKeyValueStoreConfigs.getKeyCompare(),
                            SyncKeyValueStoreConfigs::NORMAL
                            ));
                break;
                /*
            case BERKELEYDB2 :
                this->syncKeyValueStore =  new BerkeleyDB2Adapter(
                        SyncKeyValueStoreConfigs(
                            localKeyValueStoreConfigs.getDevicePath(),
                            localKeyValueStoreConfigs.getStoreSize(),
                            localKeyValueStoreConfigs.getKeyCompare(),
                            SyncKeyValueStoreConfigs::NORMAL
                            ));
                break;
                */
            case TOKYOCABINET :
                this->syncKeyValueStore =  new TokyoCabinetAdapter(
                        SyncKeyValueStoreConfigs(
                            localKeyValueStoreConfigs.getDevicePath(),
                            localKeyValueStoreConfigs.getStoreSize(),
                            localKeyValueStoreConfigs.getKeyCompare(),
                            SyncKeyValueStoreConfigs::NORMAL
                            ));
                break;
            case NOOP :
                this->syncKeyValueStore = new NoopAdapter(
                        SyncKeyValueStoreConfigs(
                            localKeyValueStoreConfigs.getDevicePath(),
                            localKeyValueStoreConfigs.getStoreSize(),
                            localKeyValueStoreConfigs.getKeyCompare(),
                            SyncKeyValueStoreConfigs::NORMAL
                            ));
                break;
            case MEMMAP:
                this->syncKeyValueStore = new MemMapAdapter(
                        SyncKeyValueStoreConfigs(
                            localKeyValueStoreConfigs.getDevicePath(),
                            localKeyValueStoreConfigs.getStoreSize(),
                            localKeyValueStoreConfigs.getKeyCompare(),
                            SyncKeyValueStoreConfigs::NORMAL
                            ));
                break;
            default:
                FAILURE("Unsupported key value store");
                break;

        }
    }
    catch (Exception& e)
    {
        FAILURE("could not initialize KV store " << e);
    }

    for( uint j = 0 ; j < this->numQueues ; ++j )
    {
        SafeKVStoreQueue * requestQueue = new SafeKVStoreQueue();
        this->requestQueues.push_back( requestQueue);

        string poolName = string("Pool_") + boost::lexical_cast<string>( j);
        this->workerPools.push_back( new WorkerPool( poolName, 2, *(this->syncKeyValueStore), *requestQueue));
    }	
}	// LocalKeyValueStore


SubmitStatus LocalKeyValueStore::insert( 
		ComparableSerializable const & key ,
		Serializable const & value,
		Durability durability,    
		InsertSuccess success ,
		InsertFailure failure
		) 
{
	/// 1. create a write KV request & enqueue it
	InsertRequest* req = new InsertRequest(
			key,
			value,
                        durability,
			success,
			failure
			);

    DLOG(1, " LocalKeyValueStore pushing insert request and waking up a thread");
	uint index = queueIndex( req);
	this->requestQueues[index]->safe_push_back(req);

	/// 2. return submit accepted
	return SubmitStatus(SUBMISSIONSTATUS_ACCEPTED) ;
}	// insert

SubmitStatus LocalKeyValueStore::lookup(
		ComparableSerializable const & key ,
		Serializable& value, 
		LookupSuccess success,
		LookupFailure failure
		) 
{
    DLOG(1, " LocalKeyValueStore pushing lookup request and waking up a thread");
	/// 1. create a lookup KV request & enqueue it
	LookupRequest* req = new LookupRequest(
			key,
			value,
			success,
			failure ) ;

	uint index = queueIndex( req);
	this->requestQueues[index]->safe_push_back(req);

	/// 2. return submit accepted
	return SubmitStatus(SUBMISSIONSTATUS_ACCEPTED) ;
}	// lookup

SubmitStatus LocalKeyValueStore::remove(
		ComparableSerializable const & key,
		RemoveSuccess success ,
		RemoveFailure failure
		) 
{
    DLOG(1, " LocalKeyValueStore pushing remove request and waking up a thread");
	/// 1. create & enqueue request
	RemoveRequest* req = new RemoveRequest(
			key,
			success,
			failure ) ;

	uint index = queueIndex( req);
	this->requestQueues[index]->safe_push_back(req);

	return SubmitStatus(SUBMISSIONSTATUS_ACCEPTED) ;
}	// remove

SubmitStatus LocalKeyValueStore::reset(ResetSuccess success,
        ResetFailure failure)
{
	ResetRequest* req = new ResetRequest(
			success,
			failure ) ;

	uint index = queueIndex( req);
	this->requestQueues[index]->safe_push_back(req);

	return SubmitStatus(SUBMISSIONSTATUS_ACCEPTED) ;
}	// reset

void LocalKeyValueStore::printStats()
{
    this->syncKeyValueStore->printStats();
}

LocalKeyValueStore::~LocalKeyValueStore()
{
	/// each adapter should have its own destructor
	delete syncKeyValueStore ;

	/// Delete all the pools first
	for( vector<WorkerPool *>::iterator j = this->workerPools.begin() ; j != this->workerPools.end() ; ++j)
    {
	  delete *j;
	}

	/// Delete all the queues
	for( vector<SafeKVStoreQueue *>::iterator j = this->requestQueues.begin() ; j != this->requestQueues.end() ; ++j)
    {
	  delete *j;
	}
}

uint32 LocalKeyValueStore::queueIndex( AbstractKVStoreRequest * ptr)
{
    static uint insertCounter = 0 ;
    static uint nonInsertCounter = 0 ;

    uint const value = (ptr->getType() == INSERT) 
        ? (((++ insertCounter) << 1) + 1)
        : (((++ nonInsertCounter) << 1) + 0);
    switch( this->numQueues)
    {
        case  1: return 0;
        case  2: return value & 1;
        case  4: return value & 3;
        case  8: return value & 7;
        case 16: return value & 15;
        default: return value % this->numQueues ;
    }
    uint8 * bytes = reinterpret_cast<uint8*>( &ptr);
    switch( this->numQueues)
    {
        case  1: return 0;
        case  2: return (bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 1;
        case  4: return (bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 3;
        case  8: return (bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 7;
        case 16: return (bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 15;
        default: return (bytes[0] + bytes[1] + bytes[2] + bytes[3]) % this->numQueues ;
    }
}


