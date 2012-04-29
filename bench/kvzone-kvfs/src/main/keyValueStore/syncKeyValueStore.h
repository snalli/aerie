/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#ifndef _KEYVALUESTORE_SYNCKEYVALUESTORE_H
#define _KEYVALUESTORE_SYNCKEYVALUESTORE_H

/********************************************************************************

  syncKeyValueStore.h
  - Synchoronous interface to Key-Value store
  - Implementation of this interface would be done by an adapter that can
  	serve a specific KV store.
  - this interface is threadsafe

 *******************************************************************************/

#include <boost/function.hpp>

#include "util/misc.h"
#include "util/exceptions.h"
#include "keyValueStore/keyValueStoreTypes.h"
#include "keyValueStore/kvStoreQueue.h"

namespace keyValueStore
{
	class SyncKeyValueStoreConfigs
	{
		public:
            enum Mode
            {
                RECOVERY = 2,
                NORMAL = 3,
            };

			SyncKeyValueStoreConfigs(
					string const &devicePath,
					uint64 storeSize,
					KeyCompare *keyCompareOp = NULL,
                    Mode mode = NORMAL 
					)
				:	devicePath(devicePath)
				,	storeSize(storeSize)
				,	keyCompareOp(keyCompareOp)
                ,   mode(mode)
			{} ;

			string const & getDevicePath() const { return  this->devicePath; } 
			uint64 getStoreSize() const { return this->storeSize; } 
			KeyCompare *getKeyCompare() const { return this->keyCompareOp; } 
            Mode getMode() const { return this->mode; } 

		private:
			string devicePath ;
			uint64 storeSize ;
			KeyCompare *keyCompareOp ;
            Mode mode;
	};

    class InsertRequest : public AbstractKVStoreRequest
    {
    public:
        InsertRequest(
                ComparableSerializable const& key,
                Serializable const& value,
                Durability durability,
                boost::function<void(ComparableSerializable const*)> const& done,
                boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> const& failure);

        virtual RequestType getType() const;

        ComparableSerializable const& getKey() const;

        Serializable const& getValue() const;

        Durability getDurability() const;

        boost::function<void(ComparableSerializable const*)> const& getSuccessCallback() const;
        
	  boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> const& getFailureCallback() const;

    private:
        ComparableSerializable const& key;
        Serializable const& value;
        Durability durability;
        boost::function<void(ComparableSerializable const*)> done;
	  boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> failure;
    };

    class RemoveRequest : public AbstractKVStoreRequest
    {
    public:
        RemoveRequest(ComparableSerializable const& key,
                boost::function<void(ComparableSerializable const*)> const& done,
					  boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> const& failure);
        
        virtual RequestType getType() const;
        
        ComparableSerializable const& getKey() const;
        
        boost::function<void(ComparableSerializable const*)> const& getSuccessCallback() const;
        
	  boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> const& getFailureCallback() const;

    private:
        ComparableSerializable const& key;
        boost::function<void(ComparableSerializable const*)> done;
	  boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> failure;
    };
    
    class ResetRequest : public AbstractKVStoreRequest
    {
    public:
        ResetRequest(
                boost::function<void()> const& done,
                boost::function<void(util::ExceptionHolder)> const& failure);
        
        virtual RequestType getType() const;
        
        boost::function<void()> const& getSuccessCallback() const;
        
        boost::function<void(util::ExceptionHolder)> const& getFailureCallback() const;

    private:
        boost::function<void()> done;
        boost::function<void(util::ExceptionHolder)> failure;
    };

    class LookupRequest : public AbstractKVStoreRequest
    {
    public:
        LookupRequest(ComparableSerializable const& key,
                Serializable& value,
                boost::function<void(ComparableSerializable const*, LookupResult)> const& done,
                boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> const& failure);
        
        virtual RequestType getType() const;
        
        ComparableSerializable const& getKey() const;
        
        Serializable & getValue() ;
       
        boost::function<void(ComparableSerializable const*, LookupResult)> const& getSuccessCallback() const;
        boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> const& getFailureCallback() const;

    private:
        ComparableSerializable const& key;
        Serializable& value;
        boost::function<void(ComparableSerializable const*, LookupResult)> done;
        boost::function<void(ComparableSerializable const*, util::ExceptionHolder)> failure;
    };

	/**
	 * Abstract base class for a generic key value store
	 */
	class SyncKeyValueStore
	{

		public:
			/**
			 * Constructor
			 * if the config options include a comparision operator on KeyType, the kv store uses
			 * this operator for indexing purposes
			 *
			 * @param syncKeyValueStoreConfig		config options
             * @param requestQueue requestQueue is a safe queue that will be
             *                      populated with work

			 *
			 * @exception Exception, ExceptionKVStoreVersionMismatch, 
			 *			ExceptionDeviceFailure, ExceptionInvalidArgument
			 */
			SyncKeyValueStore( SyncKeyValueStoreConfigs const & syncKeyValueStoreConfig);

			/**
			 * Destructor
			 */
			virtual ~SyncKeyValueStore();

            /**
             * Pops at least one request off of the request queue and processes
             * it. Adapters may optionally pop one or more requests if they can
             * be batched together. If after popping it is determined that they
             * cannot be batched requests are pushed back on head of the queue
             */
            virtual void processRequests(char* buffer, SafeKVStoreQueue &, boost::mutex::scoped_lock &queueLock) = 0;


			/**
			 * Make all inserted records persistent
			 *
			 * @exception ExceptionDeviceFailure, ExceptionInvalidArgument
			 */
			virtual void commit() = 0 ;

            /**
             *
             */
            virtual void printStats() {};
	};

} // namespace keyValueStore

#endif //_KEYVALUESTORE_SYNCKEYVALUESTORE_H


