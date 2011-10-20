#ifndef _HASHTABLE_H_HAJ123
#define _HASHTABLE_H_HAJ123

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "client/backend.h"
#include "common/debug.h"


class Context {

};

typedef int (*SplitFunction)(Context* ctx, const char* key, int keysize, void* uargs); 

const int PAGE_SIZE = 128;
const int TAG_SIZE = 1;
const int FACTOR = 1;
const int VAL_SIZE = sizeof(uint64_t);

const int NUM_BUCKETS = 128;

// PCM is treated as volatile??? this should be guaranteed by the PCM assembly macros
#define volatile 


// The code here assumes a LITTLE ENDIAN machine
//
// BUCKET
// ======
//
// +---+---------+---+--------...---+----+
// |TAG| PAYLOAD |TAG|              |NEXT|
// +---+---------+---+--------...---+----+
//
// TAG
// ===
// 
// if free (A==0):
// 
// +-+--------+
// |A|AVLSPACE|
// +-+--------+
//
//   A:        alloc flag
//   AVLSPACE: available space (7 bits)
//
// if alloc (A==1):
//
// +-+------+------+
// |A|KEYSZ |VALSZ | 
// +-+------+------+
//
//   A:     alloc flag (1 bit)
//   KEYSZ: key size (7 bits)
//   VALSZ: val size (0 bits)



class Entry {
public:
	static inline Entry* MakeEntry(volatile char* b) {
		return (Entry*) b;
	}
	static inline Entry* MakeEntry(Entry* entry, int offset) {
		Entry*          new_entry;
		volatile char*  b;
		int             new_payload_size;

		if (!entry->IsFree() ||
		    offset > entry->get_payload_size() - TAG_SIZE + 1) 
		{
			return NULL;
		}
		b = entry->tag_;
		new_entry = (Entry*) &b[offset];
		new_payload_size = entry->get_payload_size() - (offset - TAG_SIZE) - TAG_SIZE;
		new_entry->set_free(new_payload_size);
		return new_entry;
	}
	int Init(bool free, int payload_size) {
		assert(payload_size < (TAG_SIZE << 7));
		char   tag_alloc_bit = (free==true) ? 0x00 : 0x80;
		tag_[0] = tag_alloc_bit | payload_size;
		return 0;
	}
	bool IsFree() {
		return (tag_[0] & 0x80 ? false : true);
	}
	void set_free() {
		tag_[0] &= 0x7F;
	}
	void set_free(int payload_size) {
		assert(payload_size < (TAG_SIZE << 7)); 
		tag_[0] = payload_size;
	}
	int get_size() { 
		return (tag_[0] & 0x7F) + TAG_SIZE; 
	}
	int get_payload_size() { 
		return tag_[0] & 0x7F; 
	}
	void set_size(int size) { 
		assert(size-TAG_SIZE < (TAG_SIZE << 7)); 
		tag_[0] = (tag_[0] & 0x80) | (size-TAG_SIZE); 
	}
	void set_payload_size(int size) { 
		assert(size < (TAG_SIZE << 7)); 
		tag_[0] = (tag_[0] & 0x80) | size; 
	}
	int set_kv(const char* key, int key_size, const char* val, int val_size) {
		int payload_size = key_size + val_size;
		int max_payload_size = get_payload_size();
		
		if (!IsFree()) {
			return -1;
		}
		if (max_payload_size < payload_size) {
			return -1;
		}
		memcpy((void*) payload_, key, key_size);
		memcpy((void*) &payload_[key_size], val, val_size);
		tag_[0] = 0x80 | payload_size; // mark as allocated and set size in a single op
		return 0;
	}
	char* get_key() { 
		return (char*) payload_; 
	}
	int get_keysize() { 
		return get_payload_size() - VAL_SIZE; 
	}
	char* get_val() { 
		return (char*) &payload_[get_keysize()];
	}
	int get_valsize() {
		return VAL_SIZE;
	}
	char get_tag() {
		return tag_[0];
	}

	// Assumes callee checked boundary is within bucket_page
	inline Entry* NextEntry() {
		return Entry::MakeEntry(&tag_[get_size()]);
	}
private:
	volatile char tag_[TAG_SIZE];
	volatile char payload_[];
};


class Page {

public:

	Page()
	{
		Init();
	}

	int Init() {
		Entry* entry;
		int payload_size = PAGE_SIZE - sizeof(next_) - TAG_SIZE;
		next_ = 0x0;
		entry=Entry::MakeEntry(&b_[0]);
		return entry->Init(true, payload_size);
	}	

	void* operator new( size_t nbytes, client::StorageManager* sm)
	{
		void* ptr;
		int   ret;

		if ((ret = sm->Alloc(nbytes, typeid(Page), &ptr)) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}


	Page* MakePage(char* b) {
		Page* page = (Page*) b;
		return page;
	}

	Entry* GetEntry(int pos) {
		return Entry::MakeEntry(&b_[pos]);
	}
	
	bool IsEmpty() {
		Entry* entry = Entry::MakeEntry(&b_[0]);
		return (entry->get_size() == PAGE_SIZE - sizeof(next_) 
		        ? true : false);
	}

	Page* Next() {
		return (Page*) next_;
	}

	void set_next(Page* next) {
		next_ = (uint64_t) next;
	}


	int Insert(Context* ctx, const char* key, int key_size, uint64_t val);
	int Insert(Context* ctx, const char* key, int key_size, const char* val, int val_size);
	int Search(Context* ctx, const char* key, int key_size, uint64_t* val);
	int Search(Context* ctx, const char* key, int key_size, char** val, int* val_size);
	int Delete(Context* ctx, char* key, int key_size);
	int SplitHalf(Context* ctx, Page* splitover_page);
	int Split(Context* ctx, Page* splitover_page, SplitFunction split_function, void* uargs);
	int Merge(Context* ctx, Page* other_page);
	void Print();

private:
	union {
		struct {
			volatile uint64_t u64_[PAGE_SIZE/sizeof(uint64_t) - 1];
			volatile uint64_t next_;
		};
		volatile char b_[PAGE_SIZE];
	};

	int Delete(Context* ctx, Entry* entry, Entry* prev_entry, Entry* next_entry);
};



// Class Bucket encapsulates the primary bucket page
class Bucket {

public:

	Bucket()
	{
		Init();
	}

	int Init() {

		return 0;
	}	
	
	inline bool IsEmpty() {

	}

	int Insert(Context* ctx, const char* key, int key_size, uint64_t val);
	int Insert(Context* ctx, const char* key, int key_size, const char* val, int val_size);
	int Search(Context* ctx, const char* key, int key_size, uint64_t* val);
	int Search(Context* ctx, const char* key, int key_size, char** val, int* val_size);
	int Delete(Context* ctx, char* key, int key_size);
	int Split(Context* ctx, Bucket* new_bucket, SplitFunction split_function, void* uargs);
	int Merge(Context* ctx, Bucket*);
	void Print();

private:
	Page page_;

	int Delete(Context* ctx, Entry* entry, Entry* prev_entry, Entry* next_entry);
};






class HashTable {

public:
	HashTable()
		: split_idx_(0),
		  size_log2_(5)
	{ }

	void* operator new( size_t nbytes, client::StorageManager* sm)
	{
		void* ptr;
		int   ret;

		if ((ret = sm->Alloc(nbytes, typeid(HashTable), &ptr)) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}


	int Init();

	int Insert(Context* ctx, const char* key, int key_size, const char* val, int val_size);
	int Insert(Context* ctx, const char* key, int key_size, uint64_t val);
	int Search(Context* ctx, const char *key, int key_size, char** valp, int* val_sizep);
	int Search(Context* ctx, const char* key, int key_size, uint64_t* val);
	int Delete(Context* ctx, char* key, int key_size);
	void Print();

private:
	inline uint32_t Index(Context* ctx, const char* key, int key_size);

	uint32_t size_log2_;    // log2 of size 
	uint32_t split_idx_;
	
	Bucket buckets_[NUM_BUCKETS];
};


#endif /* _HASHTABLE_H_HAJ123 */
