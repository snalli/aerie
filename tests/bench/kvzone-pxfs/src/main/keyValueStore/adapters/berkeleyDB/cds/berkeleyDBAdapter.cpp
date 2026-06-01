/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 

/****************************************************************
  berkeleyDBAdapter.cpp
  - implementation does not contain any locking but is
  	thread safe because the only variables which can cause race
	conditions are Db & Env. These variables are thread safe
	internally

 ****************************************************************/

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "util/boostTime.h"
#include "berkeleyDBAdapter.h"

#define		MAX_BLOCK_SIZE		(256*1024)
#define		MAX_NUM_RETRIES		(100)

#define		INTERVAL_FOR_LOCK_CHECKING		(1) 		// seconds

// #define 	BDB_DEBUG

using namespace util;
using namespace keyValueStore::adapters::berkeleyDBAdapter ;


BerkeleyDBAdapter::BerkeleyDBAdapter( 
		SyncKeyValueStoreConfigs const & syncKeyValueStoreConfigs)
	:	SyncKeyValueStore(syncKeyValueStoreConfigs),
        numInserts(0), totalInsertLatency(0),
        numLookups(0), totalLookupLatency(0),
        numDeletes(0), totalDeleteLatency(0),
		myEnv(0),
		db(NULL)
{
    namespace fs = boost::filesystem;

	fs::path full_path( syncKeyValueStoreConfigs.getDevicePath().c_str() ) ;

	this->envname = string(full_path.branch_path().string() ) ;
	this->fname = string(full_path.leaf()) ;

	u_int32_t envFlags = 		DB_CREATE 		
							|	DB_INIT_CDB
							|	DB_INIT_MPOOL
							|	DB_THREAD
							; 

	/// if the database does not already exist, create it
	u_int32_t dbFlags = 		DB_CREATE
							|	DB_THREAD
							;

	try
	{
		/// set max amt of time a txn can live to 1sec
		// this->myEnv.set_timeout(20000000, DB_SET_TXN_TIMEOUT) ;
		// this->myEnv.set_timeout(20000000, DB_SET_LOCK_TIMEOUT) ;

		/// set lock detection; and in case a lock conflict occurs, a randomly
		/// chosen txn is killed.
		// this->myEnv.set_lk_detect( DB_LOCK_DEFAULT ) ;

		/// set max no of concurrent txns to 100
		// this->myEnv.set_tx_max(100) ;

		/// set max cache size to 0GB, 64MB, and as 1 contiguous region
		/// (use set_cache_max if needed)
		this->myEnv.set_cachesize( 0, 64*1024*1024, 1 ) ;

		/*
		/// set logging region size to 100MB
		/// if you want to restrict a single log file size, use: set_lg_max
		/// TODO: this way of setting max log size doesn't seem to work
		myEnv.set_lg_regionmax( 100*1024*1024) ;
		*/

		/**
		  * setting max total log size seems to be complicated; BDB keeps
		  * a log of all transactions, as well as can take a periodic dump
		  * of the complete database. These logs & dumps can be moved to magnetic
		  * disks. (see BDB's log config tutorial on how to do it)
		  * We need to figure out checkpointing and removal of old logs inorder to
		  * fix the size of the total log.
		  */

		/// all initializations should happen before open is called on the env
		this->myEnv.open( envname.c_str(), envFlags, 0) ;

		this->db = new Db( &myEnv, 0) ;

		/// if a key comparision function was supplied, enroll it with the BDB
		/// option currently not included.
		// db->set_bt_compare( syncKeyValueStoreConfigs.getKeyCompare() ) ;

		/// TODO: this seems to be the max page size?
		// this->db->set_pagesize(64*1024) ;

		LOG(INFO, "to open db" ) ;

		/// Open the database
		this->db->open(NULL,
				fname.c_str(),
				NULL,
				DB_BTREE,
				dbFlags,
				0
			   ) ;

	}
	catch(DbException &e)
	{
		switch( e.get_errno() )
		{
			case EINVAL :
			THROW(ExceptionInvalidArgument, e.what() ) ;
			break ;

			case ENOENT :
			THROW(ExceptionDeviceNotFound, e.what() ) ;
			break ;

			case EEXIST :
			THROW(ExceptionDeviceNonEmpty, e.what() ) ;
			break ;

			case DB_LOCK_NOTGRANTED :
			THROW(ExceptionDeviceNotFree, e.what() ) ;
			break ;

			case DB_VERSION_MISMATCH :
			THROW(ExceptionKVStoreVersionMismatch, e.what() ) ;
			break ;

			case DB_RUNRECOVERY :
			/// TODO: figure out recovery part
			/// for now, we'll throw the default exception
			THROW(keyValueStore::Exception, e.what() ) ;
			break ;

			default:
			THROW(keyValueStore::Exception, e.what() ) ;
		}
	}

	/// db & env opened successfully

	LOG(INFO, "open successful" ) ;

} // BerkeleyDBAdapter

// destructor
BerkeleyDBAdapter::~BerkeleyDBAdapter()
{
	try
	{
		/// close is also the destructor of db
		this->db->close(0) ;

		this->myEnv.close(0) ;
	}
	catch(DbException &e)
	{
		LOG( ERROR, "Error closing BDB Database & Env: " << fname << ", " << envname ) ;
	}
} // ~BerkeleyDBAdapter


void BerkeleyDBAdapter::processRequests(char* buffer,
        SafeKVStoreQueue& requestQueue, boost::mutex::scoped_lock& queueLock)
{
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
                InsertRequest& r = dynamic_cast<InsertRequest&>(*req);
                this->insert(r, buffer);
            }
            break;
        case REMOVE:
            {
                RemoveRequest& r = dynamic_cast<RemoveRequest&>(*req);
                this->remove(r, buffer);
            }
            break;
        case LOOKUP:
            {
                LookupRequest& r = dynamic_cast<LookupRequest&>(*req);
                this->lookup(r, buffer);
            }
            break;
        default:
            FAILURE("Unknown request type");
            break;
    }

    delete req;
}

void BerkeleyDBAdapter::insert( 
        InsertRequest& r, 
        char* buffer)
{
    ++this->numInserts;
    uint64 startTime = util::FastTime::getMicroseconds();

    uint32 keySize = r.getKey().getSerializedSize(); 
    uint32 valSize = r.getValue().getSerializedSize();

    char* keyBuf = buffer;
    char* valBuf = buffer + keySize;

    r.getKey().serialize(keyBuf);
    r.getValue().serialize(valBuf);

	Dbt K, V ;

	K.set_data(keyBuf) ;
	K.set_size(keySize) ;
	V.set_data(valBuf) ;
	V.set_size(valSize) ;

    try
    {
        // this->db->put( txn, &K, &V, 0 ) ;
        this->db->put( NULL, &K, &V, 0 ) ;
    }
    catch( DbException &e)
    {
        switch( e.get_errno() )
        {
            case EINVAL :
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionInvalidArgument, e.what())));
                break ;

            case ENOSPC :
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionOutOfSpace, e.what())));
                break ;

            default :
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionDeviceFailure, e.what())));
                break;
        }
        this->totalInsertLatency += util::FastTime::getMicroseconds() - startTime;
        return;
    }
        
    this->totalInsertLatency += util::FastTime::getMicroseconds() - startTime;
    r.getSuccessCallback()(&(r.getKey()));

} // insert


/// does nothing currently because our implementation is txn only & hence includes
/// commit after every single write
void BerkeleyDBAdapter::commit()
{
    /// TODO: figure out what goes in here
} // commit


/// 
void BerkeleyDBAdapter::lookup(
        LookupRequest& r,
        char* buffer)
{
    ++this->numLookups;
    uint64 startTime =  util::FastTime::getMicroseconds();
    
    uint32 keySize = r.getKey().getSerializedSize(); 

    char* keyBuf = buffer;
    char* valBuf = buffer + keySize;
   
    r.getKey().serialize(keyBuf);

    //XXX hack
    uint32 valuePtrLen = 512 * 1024;

    Dbt K( keyBuf, keySize ), V ;

	V.set_data( static_cast<void *>(valBuf) ) ;
	V.set_ulen(valuePtrLen) ;
	V.set_flags(DB_DBT_USERMEM) ;

    int ret = 0;

    try
    {
        /// get will return DB_NOTFOUND instead of throwing an exception if the
        /// (key,value) doesn't exist in db; TODO: need to handle this
        ret = this->db->get( NULL, &K, &V, 0 ) ;
    }
    catch( DbException &e)
    {
        switch( e.get_errno() )
        {
            case DB_BUFFER_SMALL :
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionMemoryUndersized, e.what())));
                break ;

            default :
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionDeviceFailure, e.what())));
                break;
        }
        this->totalLookupLatency += util::FastTime::getMicroseconds() - startTime;
        return;
    }

    this->totalLookupLatency += util::FastTime::getMicroseconds() - startTime;
    if (ret == DB_NOTFOUND)
    {
        r.getSuccessCallback()(&(r.getKey()), NOT_FOUND);
    }
    else
    {
        r.getValue().deserialize(valBuf);
        r.getSuccessCallback()(&(r.getKey()), FOUND);
    }

} // lookup


void BerkeleyDBAdapter::reset()
{
}

/// 
void BerkeleyDBAdapter::remove(RemoveRequest& r, char* buffer)
{
    ++this->numDeletes;
    uint64 startTime =  util::FastTime::getMicroseconds();
    
    uint32 keySize = r.getKey().getSerializedSize(); 

    char* keyBuf = buffer;
   
    r.getKey().serialize(keyBuf);
    
    Dbt K( keyBuf , keySize ) ;

    try
    {
        this->db->del( NULL, &K, 0 ) ;
    }
    catch( DbException &e)
    {
        this->totalDeleteLatency += util::FastTime::getMicroseconds() - startTime;
        switch( e.get_errno() )
        {
            case ENOENT :
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionValueNonExistent, e.what())));
                break ;

            case EINVAL :
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionInvalidArgument, e.what())));
                break ;

            default :
                r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionDeviceFailure, e.what())));
                break;
        }
        return;
    }
        
    this->totalDeleteLatency += util::FastTime::getMicroseconds() - startTime;
    r.getSuccessCallback()(&(r.getKey()));
    return;

} // remove





