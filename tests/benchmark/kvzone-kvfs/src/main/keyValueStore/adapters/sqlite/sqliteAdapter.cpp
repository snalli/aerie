/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 

/****************************************************************
  sqliteAdapter.cpp
  - sqlite needs to be compiled in thread safe mode for this
    program to remain so
	( to compile sqlite in threadsafe do
		./configure --enable-threadsafe
		at configuration time)

 ****************************************************************/

#include <cstring>
#include "util/boostTime.h"
#include "sqliteAdapter.h"

using boost::function;
using namespace util;
using namespace keyValueStore::adapters::sqliteAdapter ;

// constructor
SqliteAdapter::SqliteAdapter( 
		SyncKeyValueStoreConfigs const & syncKeyValueStoreConfigs)
	:	SyncKeyValueStore(syncKeyValueStoreConfigs),
        db(NULL),
        numInserts(0), totalInsertLatency(0),
        numLookups(0), totalLookupLatency(0),
        numDeletes(0), totalDeleteLatency(0)
{
	// assert( sqlite3_threadsafe() != 0 ) ;

	this->fname = syncKeyValueStoreConfigs.getDevicePath() ;

	if( sqlite3_open(this->fname.c_str(), &this->db ) != SQLITE_OK )
	{
		std::cerr << "unable to open file: " << fname.c_str() << std::endl ;
		sqlite3_close(db) ;
		THROW(ExceptionDeviceFailure, "sqlite constructor: unable to open db") ;
	}

	char *zErrMsg ;
	std::string sqlquery = "create table KV(key blob PRIMARY KEY, value blob); " ;
	int rc = sqlite3_exec(db, sqlquery.c_str(), NULL, 0, &zErrMsg);
	if( zErrMsg )
	{
		std::cout << "sqlite constructor: " << zErrMsg << std::endl ;
		free(zErrMsg) ;
	}

    if (rc != SQLITE_OK)
    {
		THROW(ExceptionDeviceFailure, "sqlite constructor: unable to create table") ;
    }

} // SqliteAdapter


// destructor
SqliteAdapter::~SqliteAdapter()
{
	sqlite3_close(db) ;

} // ~SqliteAdapter

void SqliteAdapter::processRequests(char* buffer, SafeKVStoreQueue& requestQueue, boost::mutex::scoped_lock& queueLock)
{
    DLOG(1, "SqliteAdapter processRequests");
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
                DLOG(1, "SqliteAdapter InsertRequest");
                InsertRequest& r = dynamic_cast<InsertRequest&>(*req);
                this->insert(r, buffer);
            }
            break;
        case REMOVE:
            {
                DLOG(1, "SqliteAdapter RemoveRequest");
                RemoveRequest& r = dynamic_cast<RemoveRequest&>(*req);
                this->remove(r, buffer);
            }
            break;
        case LOOKUP:
            {
                DLOG(1, "SqliteAdapter LookupRequest");
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


void SqliteAdapter::insert(
        InsertRequest& r,
        char* buffer)
{
    ++this->numLookups;
    uint64 startTime =  util::FastTime::getMicroseconds();

	sqlite3_stmt *plineInfo = 0 ;
	int rc ;

	std::string sqlquery = "insert into KV(key,value) values(?1, ?2); " ;

	/// change prepare to prepare_v2
	/// TODO: figure out if we can do prepare only once for every insert query
	rc = sqlite3_prepare( this->db, sqlquery.c_str(), sqlquery.size(), &plineInfo, NULL) ;

    if (rc != SQLITE_OK)
    {
	    /// throw any errors
	    processSQLiteErrorCode( rc, this->db,
                r.getFailureCallback(),
                &(r.getKey()),
                this->totalInsertLatency,
                startTime) ;
        return;
    }

	if( plineInfo != NULL )
	{

        uint32 keySize = r.getKey().getSerializedSize(); 
        uint32 valSize = r.getValue().getSerializedSize(); 

        char* keyBuf = buffer;
        char* valBuf = buffer + keySize;

        r.getKey().serialize(keyBuf);
        r.getValue().serialize(valBuf);

		rc = sqlite3_bind_blob( plineInfo, 1, keyBuf, keySize , SQLITE_STATIC) ;
        if (rc != SQLITE_OK)
        {
		    this->processSQLiteErrorCode(rc, this->db, r.getFailureCallback(), 
                    &(r.getKey()),
                    this->totalInsertLatency, startTime) ;
            return;
        }

		rc = sqlite3_bind_blob( plineInfo, 2, valBuf, valSize , SQLITE_STATIC) ;
        if (rc != SQLITE_OK)
        {
		    this->processSQLiteErrorCode(rc, this->db, r.getFailureCallback(), 
                    &(r.getKey()),
                    this->totalInsertLatency, startTime) ;
            return;
        }

		rc = sqlite3_step( plineInfo) ;
        if (rc != SQLITE_DONE)
        {
		    this->processSQLiteErrorCode(rc, this->db, r.getFailureCallback(), 
                    &(r.getKey()),
                    this->totalInsertLatency, startTime) ;
            return;
        }

		rc = sqlite3_finalize(plineInfo) ;
        if (rc != SQLITE_OK)
        {
		    this->processSQLiteErrorCode(rc, this->db, r.getFailureCallback(), 
                    &(r.getKey()),
                    this->totalInsertLatency, startTime) ;
            return;
        }

        this->totalInsertLatency += util::FastTime::getMicroseconds() - startTime;
        r.getSuccessCallback()(&(r.getKey()));
	}
	else
	{
		std::cerr << "Sqlite insert: something wrong here" << std::endl ;
		exit(1) ;
	}

} // insert


/// 
void SqliteAdapter::commit()
{
	/// TODO: figure out what goes in here
} // commit


/// free sqlite supplied memory
void SqliteAdapter::lookup(
        LookupRequest& r,
        char* buffer)
{
    ++this->numLookups;
    uint64 startTime = util::FastTime::getMicroseconds();

	/* do not return without calling sqlite_finalize */

	enum QueryResultType
	{
		MEMORY_INSUFFICIENT,
		RESULT_NOTFOUND,
		RESULT_FOUND,
		NOT_YET
	} ;

	sqlite3_stmt *plineInfo = 0 ;
	int rc ;

	std::string sqlquery = "select value from KV where KV.key = ?1; " ;

	/// prepare to prepare_v2
	rc = sqlite3_prepare(this->db, sqlquery.c_str(), sqlquery.size(), &plineInfo, NULL ) ;
    if (rc != SQLITE_OK)
    {
	    this->processSQLiteErrorCode(rc, this->db, r.getFailureCallback(),
                &(r.getKey()),
                this->totalLookupLatency, startTime) ;
        return;
    }

	const char *value = NULL ;
	uint64 valueLen = 0 ;
	QueryResultType foundKeyValue = NOT_YET ;

	if( plineInfo != NULL )
	{
        uint32 keySize = r.getKey().getSerializedSize(); 
        char* keyBuf = buffer;

        r.getKey().serialize(keyBuf);

		rc = sqlite3_bind_blob( plineInfo, 1, keyBuf, keySize, SQLITE_STATIC) ;
        if (rc != SQLITE_OK)
        {
		    this->processSQLiteErrorCode(rc, this->db, r.getFailureCallback(),
                    &(r.getKey()),
                    this->totalLookupLatency, startTime) ;
            return;
        }

		/// getting only the first row
		if( (rc = sqlite3_step(plineInfo)) == SQLITE_ROW)
		{
			/// don't issue call to sqlite3_column_bytes without calling sqlite3_column_blob
			/// TODO: how to catch error codes for tehse two functions?
			value = static_cast<const char *>(sqlite3_column_blob(plineInfo, 0)) ;
			valueLen = static_cast<uint64> ( sqlite3_column_bytes(plineInfo, 0)) ;

			// std::cout << "key: " << key << ", value: " << value << std::endl ;

            foundKeyValue = RESULT_FOUND ;

            char* valBuf = buffer + keySize;

            /// copy value into user memory
            memcpy( valBuf, value, valueLen) ;

		}
		else
		{
			foundKeyValue = RESULT_NOTFOUND ;
		}

	}
    else
	{
		std::cerr << "Sqlite insert: something wrong here" << std::endl ;
		exit(1) ;
	}


	/// deletes plineInfo memory; don't need it anymore
	/// this delete should happen only after we copy the result into our buffers
	rc = sqlite3_finalize(plineInfo) ;

    this->totalLookupLatency += util::FastTime::getMicroseconds() - startTime;
	switch( foundKeyValue )
	{
		case NOT_YET :
			FAILURE("should not see this error code");
            break;

		case MEMORY_INSUFFICIENT :
			r.getFailureCallback()(&(r.getKey()), ExceptionHolder(EXCEPTION(ExceptionMemoryUndersized, "lookup: supplied memory undersized"))) ;
            break;

		case RESULT_NOTFOUND :
            r.getSuccessCallback()(&(r.getKey()), NOT_FOUND);
            break;

		case RESULT_FOUND :
			/// create a ValueType with supplied memory and found value length
			/// & return it.
            r.getSuccessCallback()(&(r.getKey()), FOUND);
            break;
	}

} // lookup


/// 
void SqliteAdapter::remove( RemoveRequest& r,
        char* buffer)
{
    ++this->numDeletes;
    uint64 startTime =  util::FastTime::getMicroseconds();

	sqlite3_stmt *plineInfo = 0 ;
	int rc ;

	std::string sqlquery = "delete from KV where KV.key = ?1; " ;

	/// prepare to prepare_v2
	rc = sqlite3_prepare(this->db, sqlquery.c_str(), sqlquery.size(), &plineInfo, NULL ) ;
    if (rc != SQLITE_OK)
    {
	    this->processSQLiteErrorCode( rc, this->db,
                r.getFailureCallback(),
                &(r.getKey()),
                this->totalDeleteLatency,
                startTime) ;
        return;
    }

	if( plineInfo != NULL )
	{
        uint32 keySize = r.getKey().getSerializedSize(); 

        char* keyBuf = buffer;

        r.getKey().serialize(keyBuf);

		/// bind user memory for the blob in delete
		rc = sqlite3_bind_blob( plineInfo, 1, keyBuf, keySize, SQLITE_STATIC) ;
        if (rc != SQLITE_OK)
        {
		    this->processSQLiteErrorCode(rc, this->db, r.getFailureCallback(),
                    &(r.getKey()),
                    this->totalDeleteLatency,
                    startTime) ;
            return;
        }

		rc = sqlite3_step( plineInfo) ;
        if (rc != SQLITE_DONE)
        {
		    processSQLiteErrorCode(rc, this->db,
                    r.getFailureCallback(),
                    &(r.getKey()),
                    this->totalDeleteLatency,
                    startTime) ;
            return;
        }

		/// destructor for plineInfo
		rc = sqlite3_finalize(plineInfo) ;
        if (rc != SQLITE_OK)
        {
		    this->processSQLiteErrorCode(rc, this->db,
                    r.getFailureCallback(),
                    &(r.getKey()),
                    this->totalDeleteLatency,
                    startTime) ;
            return;
        }
    
        this->totalDeleteLatency += util::FastTime::getMicroseconds() - startTime;
        r.getSuccessCallback()(&(r.getKey()));
	}
    else
	{
		std::cerr << "Sqlite insert: something wrong here" << std::endl ;
		exit(1) ;
	}

} // remove

/// process the return value of an sqlite call and throw an exception if necessary
void SqliteAdapter::processSQLiteErrorCode( int rc, sqlite3 * /* db */,
        function<void(ComparableSerializable const* , ExceptionHolder)> const& failure,
        ComparableSerializable const* key,
        volatile uint64& totalLatency,
        uint64 startTime)
{
    totalLatency+= util::FastTime::getMicroseconds() - startTime;

	switch( rc)
	{
		case SQLITE_ERROR      :	// sql error or missing database
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: sql error or missing database")));
            break;
			
		case SQLITE_INTERNAL   :	// internal sqlite code error
			failure(key, ExceptionHolder(EXCEPTION(ExceptionDeviceFailure, "sqlite error: internal code error"))) ;
            break;

		case SQLITE_PERM       : 	// access perm denied
			failure(key, ExceptionHolder(EXCEPTION(ExceptionDeviceFailure, "sqlite error: permission denied" ))) ;
            break;

		case SQLITE_ABORT      :	// callback routine requested abort
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: callback requested abort" ))) ;
            break;

		case SQLITE_BUSY       : 	// database file is locked
			failure(key, ExceptionHolder(EXCEPTION(ExceptionValueLocked, "sqlite error: database file is locked"))) ;
            break;

		case SQLITE_LOCKED     :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionValueLocked, "sqlite error: table in the database is locked" ))) ;
            break;

		case SQLITE_NOMEM      :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionMemoryUndersized, "sqlite error: A malloc() failed" ))) ;
            break;

		case SQLITE_READONLY   :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionValueLocked, "sqlite error: Attempt to write a readonly database"))) ;
            break;

		case SQLITE_INTERRUPT  :
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: Operation terminated by sqlite3_interrupt()"))) ;
            break;

		case SQLITE_IOERR      :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionDeviceFailure, "sqlite error: Some kind of disk I/O error occurred" ))) ;
            break;

		case SQLITE_CORRUPT    :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionDeviceFailure, "sqlite error: The database disk image is malformed" ))) ;
            break;

		case SQLITE_NOTFOUND   :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionDeviceNotFound, "sqlite error: Table or record not found" ))) ;
            break;

		case SQLITE_FULL       :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionOutOfSpace, "sqlite error: insertion failed because database is full" ))) ;
            break;

		case SQLITE_CANTOPEN   :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionDeviceNotFound, "sqlite error: Unable to open database file" ))) ;
            break;

		case SQLITE_PROTOCOL   :
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: Database lock protocol error" ))) ;
            break;

		case SQLITE_EMPTY      :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionValueNonExistent, "sqlite error: Database is empty" ))) ;
            break;

		case SQLITE_SCHEMA     :
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: database schema changed" )));
            break;

		case SQLITE_TOOBIG     :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionMemoryUndersized, "sqlite error: string or blob exceeds size limit" ))) ;
            break;

		case SQLITE_CONSTRAINT :
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: Abort due to constraint violation"  ) ));
            break;

		case SQLITE_MISMATCH   :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionInvalidArgument, "sqlite error: Datatype mismatch" ) ));
            break;

		case SQLITE_MISUSE     :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionInvalidArgument, "sqlite error: Library used incorrectly" ) ));
            break;

		case SQLITE_NOLFS      :
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: Uses OS features not supported on host" ) ));
            break;

		case SQLITE_AUTH       :
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: Authorization denied" ) ));
            break;

		case SQLITE_FORMAT     :
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: format error"  ) ));
            break;

		case SQLITE_RANGE      :
			failure(key, ExceptionHolder(EXCEPTION(ExceptionInvalidArgument, "sqlite error: 2nd param to sqlite3_bind out of range" ) ));
            break;

		case SQLITE_NOTADB     :
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: not a database file" )));
            break;

		 default:
			failure(key, ExceptionHolder(EXCEPTION(Exception, "sqlite error: unknown error " << rc)));
	}
}
