/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#include "syncKeyValueStore.h"

using boost::function;

using namespace keyValueStore;
using namespace util;

SyncKeyValueStore::SyncKeyValueStore(SyncKeyValueStoreConfigs const&)
{
}

SyncKeyValueStore::~SyncKeyValueStore()
{
}

AbstractKVStoreRequest::AbstractKVStoreRequest()
{
}

AbstractKVStoreRequest::~AbstractKVStoreRequest()
{
}

InsertRequest::InsertRequest(
        ComparableSerializable const& key,
        Serializable const& value,
        Durability durability,
        function<void(ComparableSerializable const*)> const& done,
        function<void(ComparableSerializable const*, ExceptionHolder)> const& failure)
    : AbstractKVStoreRequest(),
      key(key),
      value(value),
      durability(durability),
      done(done),
      failure(failure)
{
}

RequestType InsertRequest::getType() const
{
    return INSERT;
}

ComparableSerializable const& InsertRequest::getKey() const
{
    return this->key;
}

Serializable const& InsertRequest::getValue() const
{
    return this->value;
}

Durability InsertRequest::getDurability() const
{
    return this->durability;
}

function<void(ComparableSerializable const*)> const& InsertRequest::getSuccessCallback() const
{
    return this->done;
}

function<void(ComparableSerializable const*, ExceptionHolder)> const& InsertRequest::getFailureCallback() const
{
    return this->failure;
}

ResetRequest::ResetRequest(
        function<void()> const& done,
        function<void(ExceptionHolder)> const& failure)
    : AbstractKVStoreRequest(),
      done(done),
      failure(failure)
{
}

RequestType ResetRequest::getType() const
{
    return RESET;
}

function<void()> const& ResetRequest::getSuccessCallback() const
{
    return this->done;
}

function<void(ExceptionHolder)> const& ResetRequest::getFailureCallback() const
{
    return this->failure;
}

RemoveRequest::RemoveRequest(
        ComparableSerializable const& key,
        function<void(ComparableSerializable const*)> const& done,
        function<void(ComparableSerializable const*, ExceptionHolder)> const& failure)
    : AbstractKVStoreRequest(),
      key(key),
      done(done),
      failure(failure)
{
}

RequestType RemoveRequest::getType() const
{
    return REMOVE;
}

ComparableSerializable const& RemoveRequest::getKey() const
{
    return this->key;
}

function<void(ComparableSerializable const*)> const& RemoveRequest::getSuccessCallback() const
{
    return this->done;
}

function<void(ComparableSerializable const*, ExceptionHolder)> const& RemoveRequest::getFailureCallback() const
{
    return this->failure;
}

LookupRequest::LookupRequest(
        ComparableSerializable const& key,
        Serializable& value,
        function<void(ComparableSerializable const*, LookupResult)> const& done,
        function<void(ComparableSerializable const*, ExceptionHolder)> const& failure)
    : key(key),
      value(value),
      done(done),
      failure(failure)
{
}

RequestType LookupRequest::getType() const
{
    return LOOKUP;
}

ComparableSerializable const& LookupRequest::getKey() const
{
    return this->key;
}

Serializable& LookupRequest::getValue() 
{
    return this->value;
}

function<void(ComparableSerializable const*, LookupResult)> const& LookupRequest::getSuccessCallback() const
{
    return this->done;
}

function<void(ComparableSerializable const*, ExceptionHolder)> const& LookupRequest::getFailureCallback() const
{
    return this->failure;
}
