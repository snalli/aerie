/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */
#ifndef _KEY_VALUE_STORE_H
#define _KEY_VALUE_STORE_H

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>
#include <util/exceptions.h>
#include "localKeyValueStore.h"

namespace keyValueStore
{
    using util::ExceptionHolder;
    using boost::scoped_ptr;
    using boost::function;

    template <class TKey, class TValue>
        class KeyValueStore : boost::noncopyable
    {
        public:
            /**
             * Constructor
             *
             * @param config configuration parameters
             */
            KeyValueStore(LocalKeyValueStoreConfigs const& config);

            typedef function<void(TKey const&)> InsertSuccess;
            typedef function<void(TKey const&, ExceptionHolder)> InsertFailure;

            /**
             * Inserts the given key-value pair in the key value store
             */

            SubmitStatus insert(
                    TKey const& key,
                    TValue const& value,
                    Durability durability,
                    InsertSuccess success,
                    InsertFailure failure);


            typedef function<void(TKey const&, LookupResult)> LookupSuccess;
            typedef function<void(TKey const&, ExceptionHolder)> LookupFailure;

            /**
             * Lookup up the key in the key value store
             *
             * value is an uninitialized object that will be filled in by the key value
             * store
             *
             * If key is not found in the key value store, success callback is invoked
             * with lookupResult set to NOT_FOUND
             */

            SubmitStatus lookup(
                    TKey const& key,
                    TValue& value,
                    LookupSuccess success,
                    LookupFailure failure);

            typedef function<void(TKey const&)> RemoveSuccess;
            typedef function<void(TKey const&, ExceptionHolder)> RemoveFailure;

            SubmitStatus remove(
                    TKey const& key,
                    RemoveSuccess success,
                    RemoveFailure failure);

            typedef LocalKeyValueStore::ResetSuccess ResetSuccess;
            typedef LocalKeyValueStore::ResetFailure ResetFailure;

            SubmitStatus reset(
                    ResetSuccess success,
                    ResetFailure failure);

            void printStats() { this->localStore.printStats(); }
        private:

            LocalKeyValueStore localStore;

            class InternalKey : public ComparableSerializable
            {
            public:
                InternalKey(TKey const& key)
                    : key(key)
                {
                }
                virtual bool operator<(ComparableSerializable const& other) const
                {
                    InternalKey const& o = dynamic_cast<InternalKey const&>(other);
                    return (this->key < o.key);
                }

                virtual uint32 getSerializedSize() const
                {
                    return key.getSerializedSize();
                }

                virtual void serialize(char* buffer) const
                {
                    key.serialize(buffer);
                }

                virtual void deserialize(char const* buffer)
                {
                    const_cast<TKey&>(key).deserialize(buffer);
                }

                TKey const& key;
            };

            enum SPConst
            {
                CONST = 1,
                MUTABLE = 0,
            };

            template <SPConst TC = CONST>
            class InternalValue : public Serializable
            {
            public:
                typedef typename Select<TC, TValue const, TValue>::Result TCValue;

                explicit InternalValue(TValue const& value)
                    : value(value)
                {
                }

                explicit InternalValue(TValue& value)
                    : value(value)
                {
                }

                virtual uint32 getSerializedSize() const
                {
                    return value.getSerializedSize();
                }

                virtual void serialize(char* buffer) const
                {
                    value.serialize(buffer);
                }

                virtual void deserialize(char const* buffer) 
                {
                    const_cast<TValue&>(value).deserialize(buffer);
                }

                TCValue & value;
            };

            typedef InternalValue<CONST> ConstTValue;
            typedef InternalValue<MUTABLE> MutableTValue;

            void insertDone(ComparableSerializable const*, 
                    InternalKey* key,
                    ConstTValue* value,
                    InsertSuccess success);

            void insertFailed(ComparableSerializable const*,
                    ExceptionHolder ex,
                    InternalKey* key,
                    ConstTValue* value,
                    InsertFailure failure);

            void removeDone(ComparableSerializable const*,
                    InternalKey* key,
                    RemoveSuccess success);

            void removeFailed(ComparableSerializable const*,
                    ExceptionHolder ex,
                    InternalKey* key,
                    RemoveFailure failure);

            void lookupDone(ComparableSerializable const*,
                    LookupResult result,
                    InternalKey* key,
                    MutableTValue* value,
                    LookupSuccess success);

            void lookupFailed(ComparableSerializable const*,
                    ExceptionHolder ex,
                    InternalKey* key,
                    MutableTValue* value,
                    LookupFailure failure);

            void resetDone(ResetSuccess success);
            void resetFailed(ExceptionHolder ex, ResetFailure failure);
    };

    //=======================================================================================================
    //================================================  IMPL ================================================
    //=======================================================================================================


    template <class TKey, class TValue> 
        KeyValueStore<TKey, TValue>::KeyValueStore(LocalKeyValueStoreConfigs const& config)
        : localStore(config)
        {
        }

    template <class TKey, class TValue> 
        SubmitStatus KeyValueStore<TKey, TValue>::reset(
                ResetSuccess success,
                ResetFailure failure)
        {
            DLOG(1, " KeyValueStore reset");
            return(this->localStore.reset(
                        bind(&KeyValueStore<TKey, TValue>::resetDone, this, success), 
                        bind(&KeyValueStore<TKey, TValue>::resetFailed, this, _1, failure)));
        }

    template <class TKey, class TValue> 
        void KeyValueStore<TKey, TValue>::resetDone(
                ResetSuccess success)
        {
            success();
        }

    template <class TKey, class TValue> 
        void KeyValueStore<TKey, TValue>::resetFailed(
                ExceptionHolder ex,
                ResetFailure failure)
        {
            failure(ex);
        }

    template <class TKey, class TValue> 
        SubmitStatus KeyValueStore<TKey, TValue>::insert(
                TKey const& key,
                TValue const& value,
                Durability durability,
                InsertSuccess success,
                InsertFailure failure)
        {
            DLOG(1, " KeyValueStore insert ");
            InternalKey* ik = new InternalKey(key);
            ConstTValue* iv = new ConstTValue(value); 
            return(this->localStore.insert(*ik, *iv, durability, 
                        bind(&KeyValueStore<TKey, TValue>::insertDone, this, _1, ik, iv, success), 
                        bind(&KeyValueStore<TKey, TValue>::insertFailed, this, _1, _2, ik, iv, failure)));
        }

    template <class TKey, class TValue> 
        void KeyValueStore<TKey, TValue>::insertDone(ComparableSerializable const*, 
                InternalKey* key,
                ConstTValue* value,
                InsertSuccess success)
        {
            TKey const& k = key->key;
            delete key;
            delete value;
            success(k);
        }

    template <class TKey, class TValue> 
        void KeyValueStore<TKey, TValue>::insertFailed(ComparableSerializable const*,
                ExceptionHolder ex,
                InternalKey* key,
                ConstTValue* value,
                InsertFailure failure)
        {
            TKey const& k = key->key;
            delete key;
            delete value;
            failure(k, ex);
        }

    template <class TKey, class TValue> 
        SubmitStatus KeyValueStore<TKey, TValue>::lookup(
                TKey const& key,
                TValue& value,
                LookupSuccess success,
                LookupFailure failure)
        {
            InternalKey* ikey = new InternalKey(key);
            MutableTValue* ivalue = new MutableTValue(value);
            return(this->localStore.lookup(*ikey, *ivalue, 
                        bind(&KeyValueStore<TKey, TValue>::lookupDone, this, _1, _2, ikey, ivalue, success), 
                        bind(&KeyValueStore<TKey, TValue>::lookupFailed, this, _1, _2, ikey, ivalue, failure)));
        }

    template <class TKey, class TValue> 
        void KeyValueStore<TKey, TValue>::lookupDone(ComparableSerializable const*,
                LookupResult result,
                InternalKey* key,
                MutableTValue* value,
                LookupSuccess success)
        {
            TKey const& k = key->key;
            delete key;
            delete value;
            success(k, result);
        }

    template <class TKey, class TValue> 
        void KeyValueStore<TKey, TValue>::lookupFailed(ComparableSerializable const*,
                ExceptionHolder ex,
                InternalKey* key,
                MutableTValue* value,
                LookupFailure failure)
        {
            TKey const& k = key->key;
            delete key;
            delete value;
            failure(k, ex);
        }

    template <class TKey, class TValue> 
        SubmitStatus KeyValueStore<TKey, TValue>::remove(
                TKey const& key,
                RemoveSuccess success,
                RemoveFailure failure)
        {
            InternalKey* ikey = new InternalKey(key);
            return(this->localStore.remove(*ikey, 
                        bind(&KeyValueStore<TKey, TValue>::removeDone, this, _1, ikey, success), 
                        bind(&KeyValueStore<TKey, TValue>::removeFailed, this, _1, _2, ikey, failure)));
        }

    template <class TKey, class TValue> 
        void KeyValueStore<TKey, TValue>::removeDone(ComparableSerializable const*,
                InternalKey* key,
                RemoveSuccess success)
        {
            TKey const& k = key->key;
            delete key;
            success(k);
        }

    template <class TKey, class TValue> 
        void KeyValueStore<TKey, TValue>::removeFailed(ComparableSerializable const*,
                ExceptionHolder ex,
                InternalKey* key,
                RemoveFailure failure)
        {
            TKey const& k = key->key;
            delete key;
            failure(k, ex);
        }

} // namespace keyValueStore
#endif // _KEY_VALUE_STORE_H
