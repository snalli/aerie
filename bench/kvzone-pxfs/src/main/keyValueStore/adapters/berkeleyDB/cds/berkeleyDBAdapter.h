/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#ifndef BERKELEYDB_ADAPTER_H
#define BERKELEYDB_ADAPTER_H

/***************************************************************************

  berkeleyDBAdapter.h
  - adapter for berkeley db
  - sync

 **************************************************************************/

#include <string>

#include <db_cxx.h>

#include <keyValueStore/keyValueStoreTypes.h>
#include "../../../syncKeyValueStore.h"

namespace keyValueStore
{
namespace adapters
{
namespace berkeleyDBAdapter
{
class BerkeleyDBAdapter : public SyncKeyValueStore
{

	public:

		/**
		  * constructor opens the db & env handles. all configuration needs 
		  * to be done before these handles are 'open'ed. Hence that code 
		  * will go into the constructor
		  */
		BerkeleyDBAdapter( SyncKeyValueStoreConfigs const & SyncKeyValueStoreConfigs) ;

		/// make sure Db handles are closed properly
		virtual ~BerkeleyDBAdapter() ;

        virtual void processRequests(char* buffer,
                SafeKVStoreQueue& requestQueue,
                boost::mutex::scoped_lock& queueLock);

        virtual void commit();

        virtual void reset();
	private:

        volatile uint64 numInserts;
        volatile uint64 totalInsertLatency;
        volatile uint64 numLookups;
        volatile uint64 totalLookupLatency;
        volatile uint64 numDeletes;
        volatile uint64 totalDeleteLatency;


		/// file name & env name
		std::string fname;
		std::string envname ;

		/// BDB's environment & database handles
		DbEnv myEnv ;
		Db *db ;

        void insert( InsertRequest& r,  char* buffer);
        void lookup( LookupRequest& r,  char* buffer);
        void remove( RemoveRequest& r,  char* buffer);

} ; // class BerkeleyDBAdapter 

}
}
}

#endif
