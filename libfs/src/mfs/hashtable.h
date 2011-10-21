#ifndef _HASHTABLE_H_HAJ123
#define _HASHTABLE_H_HAJ123

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "client/backend.h"
#include "common/debug.h"
#include "common/errno.h"
#include "common/hash.h"


const int HT_REHASH = 1;

inline uint64_t lgl2lnr (uint64_t logical_addr)
{
	return logical_addr;
}



class SplitPredicate {
public:
	virtual bool operator() (const char* key, int key_size) const = 0;
};

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



template<typename Context>
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


template<typename Context>
class Page {
public:
	Page()
	{
		Init();
	}

	int Init() {
		Entry<Context>* entry = Entry<Context>::MakeEntry(&b_[0]);
		int             payload_size = PAGE_SIZE - sizeof(next_) - TAG_SIZE;

		next_ = 0x0;
		return entry->Init(true, payload_size);
	}	

	void* operator new(size_t nbytes, Context* ctx)
	{
		void* ptr;
		
		if (ctx->sm->Alloc(ctx, nbytes, typeid(Page), &ptr) < 0) {
			dbg_log(DBG_ERROR, "No storage available");
		}
		return ptr;
	}

	Page* MakePage(char* b) {
		Page* page = (Page*) b;
		return page;
	}

	Entry<Context>* GetEntry(int pos) {
		return Entry<Context>::MakeEntry(&b_[pos]);
	}
	
	bool IsEmpty() {
		Entry<Context>* entry = Entry<Context>::MakeEntry(&b_[0]);
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
	int Split(Context* ctx, Page* splitover_page, const SplitPredicate& split_predicate);
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

	int Delete(Context* ctx, Entry<Context>* entry, Entry<Context>* prev_entry, Entry<Context>* next_entry);
};



// Class Bucket encapsulates the primary bucket page
template<typename Context>
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
	int Split(Context* ctx, Bucket* new_bucket, const SplitPredicate& split_predicate);
	int Merge(Context* ctx, Bucket*);
	void Print();

private:
	Page<Context> page_;

	int Delete(Context* ctx, Entry<Context>* entry, Entry<Context>* prev_entry, Entry<Context>* next_entry);
};


template<typename Context>
class HashTable {
public:
	HashTable()
		: split_idx_(0),
		  size_log2_(5)
	{ }

	void* operator new(size_t nbytes, Context* ctx)
	{
		void* ptr;
		
		if (ctx->sm->Alloc(ctx, nbytes, typeid(HashTable), &ptr) < 0) {
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
	class LinearSplit;

	inline uint32_t Index(Context* ctx, const char* key, int key_size);
	static uint32_t ModHash(uint32_t fh, int size_log2_, int shift) 
	{
		uint32_t mask = (1 << (size_log2_ + shift)) - 1;
		return fh & mask;
	}
	
	uint32_t         size_log2_;    // log2 of size 
	uint32_t         split_idx_;
	Bucket<Context>  buckets_[NUM_BUCKETS];
};


//////////////////////////////////////////////////////////////////////////////
//
//  Page
//
//////////////////////////////////////////////////////////////////////////////



// If no space available then -E_NOMEM is returned 
template<typename Context>
int 
Page<Context>::Insert(Context* ctx, const char* key, int key_size, 
                      const char* val, int val_size)
{
	Entry<Context>*  entry;
	Entry<Context>*  free_entry=NULL;
	int              payload_size = key_size + val_size;
	int              i;
	int              step;
	uint64_t         uval;

	for (i=0; i < PAGE_SIZE - (int) sizeof(next_); i+=step)
	{
		entry = GetEntry(i);
		step = entry->get_size();
		if (entry->IsFree()) {
			if (entry->get_payload_size() >= payload_size) {
				free_entry = GetEntry(i);
				break;
			}
		}
	}

	if (free_entry) {
		// Split the free entry into allocated entry and a new smaller 
		// free entry
		int max_payload_size = free_entry->get_payload_size();
		if (max_payload_size - payload_size - TAG_SIZE >= 0) {
			Entry<Context>::MakeEntry(free_entry, TAG_SIZE+payload_size);
		}

		uval = *((uint64_t*) val);
		free_entry->set_kv(key, key_size, val, val_size);
		return 0;
	}

	return -E_NOMEM;
}


// If no space available then -E_NOMEM is returned 
template<typename Context>
int 
Page<Context>::Insert(Context* ctx, const char* key, int key_size, uint64_t val)
{
	return Insert(ctx, key, key_size, (char*) &val, sizeof(val));
}


template<typename Context>
int 
Page<Context>::Search(Context* ctx, const char *key, int key_size, char** valp, 
                      int* val_sizep)
{
	int             i;
	int             step;
	Entry<Context>* entry;
	int             val_size;

	if (IsEmpty() == true) {
		return -1;
	}

	for (i=0; i < PAGE_SIZE - sizeof(next_); i+=step)
	{
		entry = GetEntry(i);
		step = entry->get_size();
		if (!entry->IsFree() && 
		    (entry->get_keysize() == key_size)) 
		{
			if (memcmp(entry->get_key(), key, key_size) == 0) {
				val_size = entry->get_valsize();
				if (valp) {
					memcpy(*valp, (char*) &b_[i+TAG_SIZE+key_size], val_size);
				}	
				if (val_sizep) {
					*val_sizep = val_size;
				}	
				return 0;
			}
		}	
	}

	return -1;
}


template<typename Context>
int 
Page<Context>::Search(Context* ctx, const char *key, int key_size, uint64_t* val)
{
	int val_size;
	return Search(ctx, key, key_size, (char**) &val, &val_size);
}


template<typename Context>
int 
Page<Context>::Delete(Context* ctx, Entry<Context>* entry, Entry<Context>* prev_entry, Entry<Context>* next_entry)
{
	int size;

	entry->set_free();
	size = entry->get_size();
	size += (next_entry && next_entry->IsFree() == true 
			 ? next_entry->get_size(): 0);

	// Coalesce contiguous empty slots
	if (prev_entry && prev_entry->IsFree() == true) {
		size += prev_entry->get_size(); 
		prev_entry->set_size(size); 
	} else {
		entry->set_size(size);
	}
	return 0;
}


template<typename Context>
int 
Page<Context>::Delete(Context* ctx, char *key, int key_size)
{
	int             i;
	int             step;
	int             size;
	Entry<Context>* entry;
	Entry<Context>* prev_entry = NULL;
	Entry<Context>* next_entry;

	for (i=0; i < PAGE_SIZE - sizeof(next_); i+=step)
	{
		entry = GetEntry(i);
		step = entry->get_size();
		if (!entry->IsFree() && 
		    (entry->get_payload_size() == key_size + VAL_SIZE)) 
		{	
			if (memcmp((char*) &b_[i+1], key, key_size) == 0) {
				// Coalesce contiguous empty slots
				if (i+step < PAGE_SIZE - sizeof(next_)) {
					next_entry = entry->NextEntry();
				} else {
					next_entry = NULL;
				}
				return Delete(ctx, entry, prev_entry, next_entry);
			}
		}
		prev_entry = entry;
	}
	return -1;
}


// Move the second half of the entries of the current bucket page into 
// the provided bucket page
//
// Callee is responsible for allocating the new bucket page
//
// If provided bucket page overflows, then remaining entries stay in the 
// current bucket page and error -E_NOMEM is returned 
template<typename Context>
int 
Page<Context>::SplitHalf(Context* ctx, Page* splitover_page)
{
	int              ret;
	int              i;
	int              size;
	Entry<Context>*  entry;
	Entry<Context>*  prev_entry = NULL;
	Entry<Context>*  next_entry;
	int              count;
	int              split_count;
	uint64_t         val;

	// count the number of entries
	for (i=0, count=0; i < PAGE_SIZE - sizeof(next_); i+=size)
	{
		entry = GetEntry(i);
		size = entry->get_size();
		if (!entry->IsFree())
		{
			count++;
		}	
	}
	if (count < 2) {
		// there are not enough entries to split
		return -1;
	}
	split_count=count/2;

	// split the bucket page in half
	for (i=0, count=0; i < PAGE_SIZE - sizeof(next_); i+=size)
	{
		entry = GetEntry(i);
		size = entry->get_size();
		if (!entry->IsFree())
		{
			count++;
			if (i+size < PAGE_SIZE - sizeof(next_)) {
				next_entry = entry->NextEntry();
			} else {
				next_entry = NULL;
			}
			if (count > split_count) {
				//FIXME: when journaling, we need to be careful about ordering and overlap
				//of inserts and deletes as reads are not bypassed through buffered 
				//writes. 
				val = *((uint64_t*) entry->get_val());
				if ((ret = splitover_page->Insert(ctx, entry->get_key(), 
				                                  entry->get_keysize(), val))!=0) 
				{
					return ret;
				}
				// It's safe to delete entries while iterating as tags
				// of follow up entries are untouched
				assert(Delete(ctx, entry, prev_entry, next_entry) == 0);
			}
		}	
		prev_entry = entry;
	}

	return 0;
}


// Split entries of the current bucket page between current page 
// and provided bucket page using provided split function
//
// Callee is responsible for allocating the new bucket page
//
// If provided bucket page overflows, then remaining entries stay in the 
// current bucket page and error -E_NOMEM is returned 
template<typename Context>
int 
Page<Context>::Split(Context* ctx, Page* splitover_page, const SplitPredicate& split_predicate)
{
	int             ret;
	int             i;
	int             size;
	Entry<Context>* entry;
	Entry<Context>* prev_entry = NULL;
	Entry<Context>* next_entry;
	uint64_t        val;
	char*           key;
	int             keysize;

	for (i=0; i < PAGE_SIZE - sizeof(next_); i+=size)
	{
		entry = GetEntry(i);
		size = entry->get_size();
		if (!entry->IsFree())
		{
			if (i+size < PAGE_SIZE - sizeof(next_)) {
				next_entry = entry->NextEntry();
			} else {
				next_entry = NULL;
			}
			key = entry->get_key();
			keysize = entry->get_keysize();
			if (split_predicate(key, keysize)) {
				//FIXME: when journaling, we need to be careful about ordering and overlap
				//of inserts and deletes as reads are not bypassed through buffered 
				//writes. 
				val = *((uint64_t*) entry->get_val());
				if ((ret = splitover_page->Insert(ctx, key, keysize, val)) != 0) {
					return ret;
				}
				// It's safe to delete entries while iterating as tags
				// of follow up entries are untouched
				assert(Delete(ctx, entry, prev_entry, next_entry) == 0);
			}
		}	
		prev_entry = entry;
	}

	return 0;
}


// Move the entries of the provided bucket page into the current bucket page.
//
// If current bucket page overflows, then remaining entries stay in the 
// provided bucket page and error -E_NOMEM is returned 
template<typename Context>
int 
Page<Context>::Merge(Context* ctx, Page* other_page)
{
	int             ret;
	int             i;
	int             size;
	Entry<Context>* entry;
	Entry<Context>* prev_entry = NULL;
	Entry<Context>* next_entry;
	int             count;
	uint64_t        val;

	for (i=0, count=0; i < PAGE_SIZE - sizeof(next_); i+=size)
	{
		entry = other_page->GetEntry(i);
		size = entry->get_size();
		if (!entry->IsFree())
		{
			if (i+size < PAGE_SIZE - sizeof(next_)) {
				next_entry = entry->NextEntry();
			} else {
				next_entry = NULL;
			}
			//FIXME: when journaling, we need to be careful about ordering and overlap
			//of inserts and deletes as reads are not bypassed through buffered 
			//writes. 
			val = *((uint64_t*) entry->get_val());
			if ((ret = Insert(ctx, entry->get_key(), 
			                  entry->get_keysize(), val)) != 0) 
			{
				return ret;
			}
			// It's safe to delete entries while iterating as tags
			// of follow up entries are untouched
			assert(other_page->Delete(ctx, entry, prev_entry, next_entry) == 0);
		}	
		prev_entry = entry;
	}

	return 0;
}


template<typename Context>
void 
Page<Context>::Print()
{
	int             i;
	uint64_t        val;
	int             size;
	Entry<Context>* entry;

	for (i=0; i < PAGE_SIZE - sizeof(next_); i+=size)
	{
		entry = GetEntry(i);
		size = entry->get_size();
		val = *((uint64_t*) entry->get_val());
		if (!entry->IsFree()) {
			printf("(%s, %llu) ", entry->get_key(), val);
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
//
//  Bucket
//
//////////////////////////////////////////////////////////////////////////////

template<typename Context>
int 
Bucket<Context>::Insert(Context* ctx, const char* key, int key_size, 
                        const char* val, int val_size)
{
	int            ret;
	Page<Context>* page;
	Page<Context>* last_page=0x0;

	// try inserting into the primary page first and otherwise
	// into an overflow page if such a page exists
	for (page=&page_; page != 0x0; page=page->Next()) {
		// FIXME: should we trigger re-hash when inserting in an 
		// existing overflow page???
		if ((ret=page->Insert(ctx, key, key_size, val, val_size)) == 0) {
			return 0;
		}
		last_page = page;
	}
	assert(last_page != 0x0);

	// No page had space. Insert the KV pair in a new page.
	
	page = new(ctx) Page<Context>;
	page->Insert(ctx, key, key_size, val, val_size);
	last_page->set_next(page);

	//TODO: trigger re-hash 

	return HT_REHASH;
}


template<typename Context>
int 
Bucket<Context>::Insert(Context* ctx, const char* key, int key_size, uint64_t val)
{
	return Insert(ctx, key, key_size, (char*) &val, sizeof(val));
}


template<typename Context>
int 
Bucket<Context>::Search(Context* ctx, const char *key, int key_size, char** valp, 
                        int* val_sizep)
{
	int            ret;
	Page<Context>* page;

	for (page=&page_; page != 0x0; page=page->Next()) {
		if ((ret=page->Search(ctx, key, key_size, valp, val_sizep)) == 0) {
			return ret;
		}
	}

	return -1;
}


template<typename Context>
int 
Bucket<Context>::Search(Context* ctx, const char* key, int key_size, uint64_t* val)
{
	int val_size;
	return Search(ctx, key, key_size, (char**) &val, &val_size);
}


template<typename Context>
int 
Bucket<Context>::Delete(Context* ctx, char* key, int key_size)
{

}


template<typename Context>
int 
Bucket<Context>::Split(Context* ctx, Bucket* splitover_bucket, 
                       const SplitPredicate& split_predicate)
{
	int            ret;
	Page<Context>* page;
	Page<Context>* new_page;
	Page<Context>* splitover_page;

	for (page=&page_; page != 0x0; page=page->Next()) {
		splitover_page = &splitover_bucket->page_;
dosplit:
		ret=page->Split(ctx, splitover_page, split_predicate);
		if (ret==-E_NOMEM) {
			if ((new_page=splitover_page->Next()) == 0x0) {
				//FIXME: allocate new page from chunk descriptor, not new/malloc
				//FIXME: protect against cycle-loop resulting from infinite splits. 
				//       is this possible? shouldn't splits converge?
				new_page = new(ctx) Page<Context>;
				splitover_page->set_next(new_page);
			}
			splitover_page = new_page;
			goto dosplit;
		}
	}
	return 0;
}


template<typename Context>
void Bucket<Context>::Print()
{
	Page<Context>* page;

	for (page=&page_; page != 0x0; page=page->Next()) {
		printf("Page: %p = {\n", page);
		page->Print();
		printf("}\n");
	}
}


//////////////////////////////////////////////////////////////////////////////
//
//  HashTable
//
//////////////////////////////////////////////////////////////////////////////


template<typename Context>
class HashTable<Context>::LinearSplit: public SplitPredicate {
public:
	LinearSplit(uint32_t size_log2)
		: size_log2_(size_log2)
	{ }

	bool operator() (const char* key, int key_size) const 
	{
		uint32_t             curidx;
		uint32_t             newidx;
		uint32_t             fh;

		fh = hashlittle(key, key_size, 0);
		curidx = ModHash(fh, size_log2_, 0);
		newidx = ModHash(fh, size_log2_, 1);
		return (newidx != curidx) ? true: false;
	}

private:
	uint32_t size_log2_;
};


template<typename Context>
int
HashTable<Context>::Init()
{
	split_idx_ = 0;
	size_log2_ = 5;
}


template<typename Context>
uint32_t 
HashTable<Context>::Index(Context* ctx, const char* key, int key_size)
{
	uint32_t idx;
	uint32_t fh;

	fh = hashlittle(key, key_size, 0);
	if ((idx = ModHash(fh, size_log2_, 0)) < split_idx_ ) {
		idx = ModHash(fh, size_log2_, 1);
	}
	return idx;
}


template<typename Context>
int 
HashTable<Context>::Insert(Context* ctx, const char* key, int key_size, 
                           const char* val, int val_size)
{
	uint32_t         idx;
	Bucket<Context>* bucket;
	Bucket<Context>* splitover_bucket;
	int              ret;

	printf("HashTable::Insert key=%s\n", key);

	idx = Index(ctx, key, key_size);
	bucket = &buckets_[idx];
	if ((ret=bucket->Insert(ctx, key, key_size, val, val_size))==0) {
		return 0;
	}
	if (ret == HT_REHASH) {
		bucket = &buckets_[split_idx_];
		splitover_bucket = &buckets_[split_idx_+(1<<size_log2_)];
		bucket->Split(ctx, splitover_bucket, LinearSplit(size_log2_));
		split_idx_++;
		if (split_idx_ == (1 << size_log2_)) {
			split_idx_ = 0;
			size_log2_++;
		}
	}
	return 0;
}


template<typename Context>
int 
HashTable<Context>::Insert(Context* ctx, const char* key, int key_size, uint64_t val)
{
	return Insert(ctx, key, key_size, (char*) &val, sizeof(val));
}


template<typename Context>
int 
HashTable<Context>::Search(Context* ctx, const char *key, int key_size, char** valp, 
                           int* val_sizep)
{
	uint32_t         idx;
	Bucket<Context>* bucket;

	printf("HashTable::Search key=%s\n", key);

	idx = Index(ctx, key, key_size);
	bucket = &buckets_[idx];
	return bucket->Search(ctx, key, key_size, valp, val_sizep);
}


template<typename Context>
int 
HashTable<Context>::Search(Context* ctx, const char* key, int key_size, 
                           uint64_t* val)
{
	int val_size;

	return Search(ctx, key, key_size, (char**) &val, &val_size);
}


template<typename Context>
void 
HashTable<Context>::Print()
{
	int              i;
	uint32_t         idx;
	Bucket<Context>* bucket;

	for (i=0; i<(1<<size_log2_)+split_idx_; i++) {
		bucket = &buckets_[i];
		bucket->Print();
	}	
}

#endif /* _HASHTABLE_H_HAJ123 */
