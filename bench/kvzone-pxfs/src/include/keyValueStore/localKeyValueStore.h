/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

/***************************************************************
  localKeyValueStore.h
  - async API that communicates with the sync API of KV store

  TODO:
  	handle namespaces
 **************************************************************/

#ifndef		_LOCAL_KEYVALUE_STORE_H
#define		_LOCAL_KEYVALUE_STORE_H

#include <vector>
#include <boost/function.hpp>

#include "util/exceptions.h"
#include "util/misc.h"

#include "keyValueStore/kvStoreQueue.h"
#include "keyValueStore/keyValueStoreTypes.h"


using namespace util ;

namespace keyValueStore
{
    class SyncKeyValueStore;
    class WorkerPool;

	enum SubmissionStatus
	{
		SUBMISSIONSTATUS_REJECTED,
		SUBMISSIONSTATUS_ACCEPTED
	} ;

	/// SubmitStatus is defined in V2 as well
	class SubmitStatus
	{
		public:
			
			SubmitStatus(SubmissionStatus submissionStatus) ;

			SubmissionStatus getStatus() const ;

		private:
			SubmissionStatus submissionStatus ;
	} ; // class SubmitStatus

	/// configuration parameters for LocalKeyValueStore
	class LocalKeyValueStoreConfigs
	{
		public:
			LocalKeyValueStoreConfigs( 
					std::string const & devicePath,
					uint16 numThreads,
                    uint64 storeSize,
                    KeyValueStoreType type,
					KeyCompare *keyCompare = NULL
					) ;

			std::string const & getDevicePath() const ;
			uint16 getNumThreads() const  ;
			KeyCompare * getKeyCompare() const ;
			uint64 getStoreSize() const ;
            KeyValueStoreType getType() const;

		private:
			std::string const devicePath ;
			uint16 const numThreads ;
            KeyValueStoreType type;
			KeyCompare * keyCompare ;
			uint64 storeSize ;
	} ;

	class LocalKeyValueStoreInitFailedException : public Exception
	{
	} ;

#define NUM_QUEUES 8

	/**
	  * not an abstract class;
	  * provides async API for accessing the KV store.
	  */
	class LocalKeyValueStore : boost::noncopyable
	{
		public:

			/// should we mention the type of KV store to use? (through config file?)
			LocalKeyValueStore( LocalKeyValueStoreConfigs const &localKeyValueStoreConfigs) ;
			
			~LocalKeyValueStore() ;
		
			/**
			  * InsertSuccess -- calls w/ the key for which a success is being reported
			  * InsertFailure -- passes the error code in addition to the key
			  */
			typedef boost::function< void(ComparableSerializable const*  ) > InsertSuccess ;
			typedef boost::function< void(ComparableSerializable const*, ExceptionHolder) > InsertFailure ;

			/**
			  * submit status will indicate so if the request was rejected sync'ly
              *
              * Note the buffer contained in key and value should be alive
              * until the callback is invoked
			  */
			SubmitStatus insert( 
					ComparableSerializable const & key,
					Serializable const & value,
					Durability durability,
					InsertSuccess success,
					InsertFailure failure
					); 

			/**
			  * LookupSuccess -- calls w/ the key for which a success is being reported; 
			  *						value is passed on as second argument
			  * LookupFailure -- passes the error code and the key
			  */
			typedef boost::function< void(ComparableSerializable const*, LookupResult ) > LookupSuccess ;
			typedef boost::function< void(ComparableSerializable const*, ExceptionHolder) > LookupFailure ;


			/**
			  * looks up Value that corresponds to input Key 
              *
              * Note the buffer contained in key and value should be alive
              * until the callback is invoked
			  * 
			  * @param key				holds the void * of the key and its length
			  * @param valuePtrLen		length of the value's void *
			  * @param success			success callback
			  * @param failure			failure callback
			  */
			SubmitStatus lookup(
					ComparableSerializable const & key,
					Serializable & value,
					LookupSuccess success,
					LookupFailure failure
					) ;

			/**
			  * RemoveSuccess -- calls w/ the key for which a success is being reported; 
			  * RemoveFailure -- passes the error code and the key
			  */
			typedef boost::function< void(ComparableSerializable const*) > RemoveSuccess ;
			typedef boost::function< void(ComparableSerializable const*, ExceptionHolder) > RemoveFailure ;

			/**
			  * remove <key,value> pair corresponding to input key
              *
              * Note the buffer contained in key and value should be alive
              * until the callback is invoked
			  * @param	key			holds the void * of the key and its length
			  * @param	success
			  * @param  failure
			  */
			SubmitStatus remove(
					ComparableSerializable const & key,
					RemoveSuccess success,
					RemoveFailure failure
					) ;

            typedef boost::function<void()> ResetSuccess;
            typedef boost::function<void(ExceptionHolder const&)> ResetFailure;

			/**
			  * reset()		cleans the database and the queue
			  */
			SubmitStatus reset(ResetSuccess success,
                    ResetFailure failure);

            void printStats();

			/// see if an iterator can be implemetned

			/// To reduce contention, each local KV store has a pool of queues, each queue serviced by a pool of threads
            vector<SafeKVStoreQueue *> requestQueues;
		private:
	        uint const numQueues;

			SyncKeyValueStore *syncKeyValueStore ;

			boost::mutex mutex_remainingStoreMemory ;
			uint64 remainingStoreMemory ;
			
            vector<WorkerPool *> workerPools ; // same size as the vector of request queues
	  
            uint32 queueIndex( AbstractKVStoreRequest * ptr);
	} ; // class LocalKeyValueStore
} ; // namespace keyValueStore
#endif
