#ifndef _REGISTRY_H_IKL111
#define _REGISTRY_H_IKL111

#include <pthread.h>
#include "common/pheap.h"
#include "common/vistaheap.h"
#include "common/uthash.h"

typedef struct REntry REntry;

struct REntry {
	char           name[128];
	void*          ptr;
	UT_hash_handle hh;
};

struct RegistryRoot {
		REntry*         entry_array_;
};

class Registry {
	public:
		Registry();
		int Init();
		int Lookup(const char*, void**);
		int Add(const char*, void*);
		int Remove(const char*);
	private:
		pthread_mutex_t mutex_;
		PHeap*          pheap_;
		RegistryRoot*   registry_root_;
};


#endif
