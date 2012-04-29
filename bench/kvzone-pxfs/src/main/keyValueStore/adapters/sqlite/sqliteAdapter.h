/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#ifndef SQLITE_ADAPTER_H
#define SQLITE_ADAPTER_H

/***************************************************************************

  sqliteAdapter.h
  - adapter for sqlite kv store 
  - sync

 **************************************************************************/

#include <string>

#include <sqlite3.h>

#include <keyValueStore/keyValueStoreTypes.h>
#include "../../syncKeyValueStore.h"

namespace keyValueStore
{
namespace adapters
{
namespace sqliteAdapter
{

class SqliteAdapter : public SyncKeyValueStore
{

	public:

		/**
		  * constructor opens the db handle. 
		  */
		SqliteAdapter( SyncKeyValueStoreConfigs const & SyncKeyValueStoreConfigs) ;

		/// make sure handles are closed properly & memory is freed; sqlite3 alloc's
		/// memory and it needs to be explicitly freed.
		virtual ~SqliteAdapter() ;

		/**
		  * insert the key & value into the db
		  */
		virtual void processRequests(
                char* buffer,
                SafeKVStoreQueue& requestQueue,
                boost::mutex::scoped_lock& queueLock);

		virtual void commit() ;
		
        virtual void reset(){}

	private:

		/// might throw an exception if necessary
		void processSQLiteErrorCode( int , sqlite3 *,
									 boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> const& failure,
                ComparableSerializable const* key,
                volatile uint64& totalLatency,
                uint64 startTime) ;

		/// file name 
		std::string fname ;

		/// sqlite3's database handles
		sqlite3 *db ;

        volatile uint64 numInserts;
        volatile uint64 totalInsertLatency;
        volatile uint64 numLookups;
        volatile uint64 totalLookupLatency;
        volatile uint64 numDeletes;
        volatile uint64 totalDeleteLatency;

        void insert(InsertRequest& r, char* buffer);

        void remove(RemoveRequest& r, char* buffer);

        void lookup(LookupRequest& r, char* buffer);

} ; // class SqliteAdapter 

}
}
}

#endif


