#ifndef __STAMNOS_OSD_COMMON_HASHTABLE_H 
#define __STAMNOS_OSD_COMMON_HASHTABLE_H 

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <typeinfo>
#include "bcs/main/common/cdebug.h"
#include "common/errno.h"
#include "common/hash.h"
#include "osd/containers/map/openhashtable.h"


struct Key {
	Key()
	{
		c[0] = '\0';
	}

	Key(const char* str)
	{
		strcpy(c, str);
	}

	char c[32];
	
	bool operator==(const Key& other) {
		return strcmp(c, other.c) == 0;
	}
};


class Traits
{
public:
	static uint hash(const char* key)
	{
		uint hash = 2166136261;
		for (const char *s = key; *s; s++)
			hash = (16777619 * hash) ^ (*s);
		return hash;
	};


	static uint hash(const Key& key)
	{
		uint hash = 2166136261;
		for (const char *s = key.c; *s; s++)
			hash = (16777619 * hash) ^ (*s);
		return hash;
	};

	static bool isEmpty(Key& key)
	{
		return  (key.c[0] == '\0') ? true: false;
	}
};


template<typename Session>
class HashTable {
	typedef OpenHashTable<Session, Key, uint64_t, Traits> OpenHT;
public:
	HashTable()
		: 
		  ncount_(0),
		  table_(0.8)
	{ }

	static HashTable* Make(Session* session)
	{
		HashTable* ht;
		void*      ptr;
		if (session->salloc()->AllocateExtent(session, sizeof(HashTable), 0, &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return new(ptr) HashTable();
	}

	static HashTable* Make(Session* session, void* buf)
	{
		HashTable* ht;
		void*      ptr;

		return new(buf) HashTable();
	}


	int Init();

	int Insert(Session* session, const char* key, int key_size, const char* val, int val_size);
	int Insert(Session* session, const char* key, int key_size, uint64_t val);
	int Search(Session* session, const char *key, int key_size, char** valp, int* val_sizep);
	int Search(Session* session, const char* key, int key_size, uint64_t* val);
	int Search(Session* session, const char* key, uint64_t* val);
	int Delete(Session* session, const char* key, int key_size);
	void Print();
	int Size(Session* session) { return ncount_; }

private:
	
	uint32_t   ncount_;               // number of entries 
	OpenHT     table_;
};


template<typename Session>
int
HashTable<Session>::Init()
{
	ncount_ = 0;
	return E_SUCCESS;
}


template<typename Session>
int 
HashTable<Session>::Insert(Session* session, const char* key, int key_size, 
                           const char* val, int val_size)
{
	assert(0);
}


template<typename Session>
int 
HashTable<Session>::Insert(Session* session, const char* key, int key_size, uint64_t val)
{
	return table_.Insert(session, Key(key), val);
}


template<typename Session>
int 
HashTable<Session>::Search(Session* session, const char *key, int key_size, 
                           char** valp, int* val_sizep)
{
	assert(0);
}


template<typename Session>
int 
HashTable<Session>::Search(Session* session, const char* key, int key_size, 
                           uint64_t* val)
{
	assert(0);
	//return table_.Lookup(session, Key(key), val);
}

template<typename Session>
int 
HashTable<Session>::Search(Session* session, const char* key, uint64_t* val)
{
	return table_.Lookup(session, key, val);
}


template<typename Session>
int 
HashTable<Session>::Delete(Session* session, const char* key, int key_size)
{

}


template<typename Session>
void 
HashTable<Session>::Print()
{
}

#endif // __STAMNOS_OSD_COMMON_HASHTABLE_H 
