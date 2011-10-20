#include "mfs/hashtable.h"
#include <stdint.h>
#include <string.h>
#include "client/backend.h"
#include "common/errno.h"
#include "common/hash.h"


const int HT_REHASH = 1;

inline uint64_t lgl2lnr (uint64_t logical_addr)
{
	return logical_addr;
}

//FIXME: allocate storage via storage manager


//////////////////////////////////////////////////////////////////////////////
//
//  Page
//
//////////////////////////////////////////////////////////////////////////////



// If no space available then -E_NOMEM is returned 
int 
Page::Insert(Context* ctx, const char* key, int key_size, 
             const char* val, int val_size)
{
	int      i;
	int      step;
	Entry*   entry;
	Entry*   free_entry=NULL;
	int      payload_size = key_size + val_size;
	uint64_t uval;

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
			Entry::MakeEntry(free_entry, TAG_SIZE+payload_size);
		}

		uval = *((uint64_t*) val);
		free_entry->set_kv(key, key_size, val, val_size);
		return 0;
	}

	return -E_NOMEM;
}


// If no space available then -E_NOMEM is returned 
int 
Page::Insert(Context* ctx, const char* key, int key_size, uint64_t val)
{
	return Insert(ctx, key, key_size, (char*) &val, sizeof(val));
}


int 
Page::Search(Context* ctx, const char *key, int key_size, char** valp, 
             int* val_sizep)
{
	int    i;
	int    step;
	Entry* entry;
	int    val_size;

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


int 
Page::Search(Context* ctx, const char *key, int key_size, uint64_t* val)
{
	int val_size;
	return Search(ctx, key, key_size, (char**) &val, &val_size);
}


int 
Page::Delete(Context* ctx, Entry* entry, Entry* prev_entry, Entry* next_entry)
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


int 
Page::Delete(Context* ctx, char *key, int key_size)
{
	int    i;
	int    step;
	int    size;
	Entry* entry;
	Entry* prev_entry = NULL;
	Entry* next_entry;

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
int 
Page::SplitHalf(Context* ctx, Page* splitover_page)
{
	int      ret;
	int      i;
	int      size;
	Entry*   entry;
	Entry*   prev_entry = NULL;
	Entry*   next_entry;
	int      count;
	int      split_count;
	uint64_t val;

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
int 
Page::Split(Context* ctx, Page* splitover_page, SplitFunction split_function, 
            void* uargs)
{
	int      ret;
	int      i;
	int      size;
	Entry*   entry;
	Entry*   prev_entry = NULL;
	Entry*   next_entry;
	uint64_t val;
	char*    key;
	int      keysize;


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
			if (split_function(ctx, key, keysize, uargs)) {
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
int 
Page::Merge(Context* ctx, Page* other_page)
{
	int      ret;
	int      i;
	int      size;
	Entry*   entry;
	Entry*   prev_entry = NULL;
	Entry*   next_entry;
	int      count;
	uint64_t val;

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


void Page::Print()
{
	int i;
	uint64_t val;
	int size;
	Entry* entry;

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

int 
Bucket::Insert(Context* ctx, const char* key, int key_size, const char* val, 
               int val_size)
{
	int   ret;
	Page* page;
	Page* last_page=0x0;

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
	
	//FIXME: allocate from chunk allocator instead, not new/malloc
	page = new(client::global_smgr) Page;
	page->Insert(ctx, key, key_size, val, val_size);
	last_page->set_next(page);

	//TODO: trigger re-hash 

	return HT_REHASH;
}


int 
Bucket::Insert(Context* ctx, const char* key, int key_size, uint64_t val)
{
	return Insert(ctx, key, key_size, (char*) &val, sizeof(val));
}


int 
Bucket::Search(Context* ctx, const char *key, int key_size, char** valp, 
               int* val_sizep)
{
	int   ret;
	Page* page;

	for (page=&page_; page != 0x0; page=page->Next()) {
		if ((ret=page->Search(ctx, key, key_size, valp, val_sizep)) == 0) {
			return ret;
		}
	}

	return -1;
}


int 
Bucket::Search(Context* ctx, const char* key, int key_size, uint64_t* val)
{
	int val_size;
	return Search(ctx, key, key_size, (char**) &val, &val_size);
}


int 
Bucket::Delete(Context* ctx, char* key, int key_size)
{

}


int 
Bucket::Split(Context* ctx, Bucket* splitover_bucket, 
              SplitFunction split_function, void* uargs)
{
	int   ret;
	Page* page;
	Page* new_page;
	Page* splitover_page;

	for (page=&page_; page != 0x0; page=page->Next()) {
		splitover_page = &splitover_bucket->page_;
dosplit:
		ret=page->Split(ctx, splitover_page, split_function, uargs);
		if (ret==-E_NOMEM) {
			if ((new_page=splitover_page->Next()) == 0x0) {
				//FIXME: allocate new page from chunk descriptor, not new/malloc
				//FIXME: protect against cycle-loop resulting from infinite splits. 
				//       is this possible? shouldn't splits converge?
				new_page = new(client::global_smgr) Page;
				splitover_page->set_next(new_page);
			}
			splitover_page = new_page;
			goto dosplit;
		}
	}
	return 0;
}


void Bucket::Print()
{
	Page* page;

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


int
HashTable::Init()
{
	split_idx_ = 0;
	size_log2_ = 5;
}


static inline uint32_t Hash(uint32_t fh, int size_log2_, int shift) 
{
	uint32_t mask = (1 << (size_log2_ + shift)) - 1;
	return fh & mask;
}


typedef struct {
	uint32_t size_log2_;
} split_function_args;


static int split_function(Context* ctx, const char* key, int key_size, void* uargs)
{
	uint32_t             curidx;
	uint32_t             newidx;
	uint32_t             fh;
	split_function_args* args = (split_function_args*) uargs;

	fh = hashlittle(key, key_size, 0);

	curidx = Hash(fh, args->size_log2_, 0);
	newidx = Hash(fh, args->size_log2_, 1);
	if (newidx != curidx) {
		return 1;
	}
	return 0;
}


inline uint32_t 
HashTable::Index(Context* ctx, const char* key, int key_size)
{
	uint32_t idx;
	uint32_t fh;

	fh = hashlittle(key, key_size, 0);
	
	if ((idx = Hash(fh, size_log2_, 0)) < split_idx_ ) {
		idx = Hash(fh, size_log2_, 1);
	}
	
	return idx;
}


int 
HashTable::Insert(Context* ctx, const char* key, int key_size, 
                  const char* val, int val_size)
{
	uint32_t idx;
	Bucket*  bucket;
	Bucket*  splitover_bucket;
	int      ret;

	printf("HashTable::Insert key=%s\n", key);

	idx = Index(ctx, key, key_size);
	bucket = &buckets_[idx];
	if ((ret=bucket->Insert(ctx, key, key_size, val, val_size))==0) {
		return 0;
	}
	if (ret == HT_REHASH) {
		split_function_args args;
		args.size_log2_ = size_log2_;
		bucket = &buckets_[split_idx_];
		splitover_bucket = &buckets_[split_idx_+(1<<size_log2_)];
		bucket->Split(ctx, splitover_bucket, split_function, (void*) &args);
		split_idx_++;
		if (split_idx_ == (1 << size_log2_)) {
			split_idx_ = 0;
			size_log2_++;
		}
	}
	return 0;
}


int 
HashTable::Insert(Context* ctx, const char* key, int key_size, uint64_t val)
{
	return Insert(ctx, key, key_size, (char*) &val, sizeof(val));
}


int 
HashTable::Search(Context* ctx, const char *key, int key_size, char** valp, 
                  int* val_sizep)
{
	uint32_t idx;
	Bucket*  bucket;

	printf("HashTable::Search key=%s\n", key);

	idx = Index(ctx, key, key_size);
	bucket = &buckets_[idx];
	return bucket->Search(ctx, key, key_size, valp, val_sizep);
}


int 
HashTable::Search(Context* ctx, const char* key, int key_size, uint64_t* val)
{
	int val_size;

	return Search(ctx, key, key_size, (char**) &val, &val_size);
}


void 
HashTable::Print()
{
	int      i;
	uint32_t idx;
	Bucket*  bucket;

	for (i=0; i<(1<<size_log2_)+split_idx_; i++) {
		bucket = &buckets_[i];
		bucket->Print();
	}	
}
