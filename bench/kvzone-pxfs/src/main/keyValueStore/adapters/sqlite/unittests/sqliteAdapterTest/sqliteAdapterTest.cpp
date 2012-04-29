/*
 * Copyright (c) 2010 NEC Laboratories, Inc. All rights reserved.
 */ 
#include "../../sqliteAdapter.h"

using namespace keyValueStore::adapters::sqliteAdapter;

void testSqliteAdapter()
{
	string devicePath("/tmp/sqliteTest");
	uint64 s = 0;

	SqliteAdapter sqlite(SyncKeyValueStoreConfigs(devicePath, s));

    uint32 key1(1);
    uint32 value1(2);

	KeyType key(reinterpret_cast<void *>(&key1), sizeof(key1));
	ValueType value(reinterpret_cast<void *>(&value1), sizeof(value1));

	sqlite.insert(key, value, keyValueStore::SyncKeyValueStore::DURABLE );

    uint32 value2(0);
	Option<ValueType> lv = sqlite.lookup(key,  reinterpret_cast<void *>(&value2), sizeof(value2));
	PASSERT(lv.hasSome(), "invalid state");
	std::cout << *(reinterpret_cast<uint32 *>(lv.getValue().getBuffer())) << std::endl ;

	DASSERT(value2 == value1, "invalid state");

	sqlite.commit();

	sqlite.remove(key);
	
	value2 = 0 ;
	Option<ValueType> lv1 = sqlite.lookup(key,  reinterpret_cast<void *>(&value2), sizeof(value2));
	PASSERT(!lv1.hasSome(), "invalid state");

	std::cout << value2 << std::endl ;
}

int main()
{
	testSqliteAdapter();
	return 0;
}


