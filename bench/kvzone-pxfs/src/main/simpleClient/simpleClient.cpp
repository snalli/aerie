/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */

#include <iostream>
#include <cstdlib>
#include <util/misc.h>
#include <util/completion.h>
#include <keyValueStore/keyValueStoreTypes.h>
#include <keyValueStore/keyValueStore.h>

using namespace std;
using namespace util;
using namespace keyValueStore;

class MyKeyType 
{
public:
    explicit MyKeyType(uint64 key)
        : key(key)
    {
    }
    
    explicit MyKeyType()
        : key(0)
    {
    }

    MyKeyType(MyKeyType const& other)
    {
        this->key = other.key;
    }

    ~MyKeyType()
    {
    }

    uint64 getKey() const
    {
        return this->key;
    }

    virtual bool operator<(MyKeyType const& other) const
    {
        return (this->key < other.key); 
    }

    bool operator==(MyKeyType const& other) const
    {
        return (this->key == other.key);
    }

    uint32 getSerializedSize() const
    {
        return (sizeof(this->key));
    }

	void serialize(char* buffer) const
    {
        memcpy(buffer, &this->key, sizeof(this->key));
    }

    void deserialize(char const* buffer)
    {
        memcpy(&this->key, buffer, sizeof(this->key));
    }

private:
    uint64 key;
};

class MyValueType 
{
public:
    explicit MyValueType(uint64 value)
        : value(value)
    {
    }
    
    explicit MyValueType()
        : value(0)
    {
    }

    MyValueType(MyValueType const& other)
    {
        this->value = other.value;
    }

    ~MyValueType()
    {
    }

    uint64 getVal() const
    {
        return this->value;
    }

    uint32 getSerializedSize() const
    {
        return sizeof(this->value);
    }

	void serialize(char* buffer) const
    {
        memcpy(buffer, &this->value, sizeof(this->value));
    }

    void deserialize(char const* buffer)
    {
        memcpy(&this->value, buffer, sizeof(this->value));
    }

private:
    uint64 value;
};

typedef MyKeyType KeyType;
typedef MyValueType ValueType;

KeyValueStore<KeyType, ValueType >* kvStore;

void writeDone(KeyType const&,
        util::Completion* cond)
{
    cond->signal();
}

void writeFailed(KeyType const&,
        ExceptionHolder ex)
{
    util::unused(ex);
    FAILURE("write failed " << ex);
}

void deleteDone(KeyType const&,
        util::Completion* cond)
{
    cond->signal();
}

void deleteFailed(KeyType const&,
        ExceptionHolder ex)
{
    util::unused(ex);
    FAILURE("delete failed " << ex);
}

void lookupDone(KeyType const&,
        LookupResult result,
        ValueType* v1,
        ValueType const& v2,
        util::Completion* cond)
{
    util::unused(result);
    util::unused(v1);
    util::unused(v2);
    PASSERT(result == keyValueStore::FOUND, "key not found");
    PASSERT(v1->getVal() == v2.getVal(), "value mismatch");
    cond->signal();
}

void lookupFailed(KeyType const&,
        ExceptionHolder ex)
{
    util::unused(ex);
    FAILURE("lookup failed " << ex);
}

int main()
{
    // completion is just a conditional variable
    util::Completion cond;

    // on disk size of the key value store
    uint32 storeSize= 10 * 1024 * 1024;

    // type of key value store
    KeyValueStoreType kvType = TOKYOCABINET;

    // on disk location of the key value store : note this can be a block
    // device instead of file for alphard
    string devicePath = "/tmp/kvstore";

    uint16_t numThreads = 10;

   
    LocalKeyValueStoreConfigs config(devicePath, 
            numThreads,
            storeSize,
            kvType);

    kvStore = new KeyValueStore<KeyType, ValueType>(config);

    KeyType k(3);
    ValueType v(5);

    cerr << "Insert key 3 value 5" << endl;
    SubmitStatus s1 = kvStore->insert(
            k,
            v,
            keyValueStore::DURABLE,
            bind(&writeDone, _1, &cond),
            bind(&writeFailed, _1, _2));

    cond.wait();

    cerr << "Lookup key 3" << endl;
    ValueType v2;
    SubmitStatus s2 = kvStore->lookup(
            k,
            v2,
            bind(&lookupDone, _1, _2, &v2, v, &cond),
            bind(&lookupFailed, _1, _2));
    cond.wait();

    cerr << "Got " << v2.getVal() << endl;

    SubmitStatus s3 = kvStore->remove(
            k,
            bind(&deleteDone, _1, &cond),
            bind(&deleteFailed, _1, _2));
    cond.wait();

    delete kvStore;

    return 0;
}

