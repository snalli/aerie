#ifndef _NAMESERVICE_H_IKL111
#define _NAMESERVICE_H_IKL111

#include <pthread.h>
#include "common/pheap.h"
#include "common/vistaheap.h"
#include "common/uthash.h"

typedef struct NEntry NEntry;

struct NEntry {
	char           name_[128];
	void*          ptr_;
	unsigned int   refcount_;
	UT_hash_handle hh;
};

struct NameServiceRoot {
		NEntry*         entry_array_;
};

class NameService {
	public:
		NameService();
		int fsck();
		int Init();
		int Lookup(const char*, void**);
		int Link(const char*, void*);
		int Unlink(const char*);
	private:
		pthread_mutex_t   mutex_;
		PHeap*            pheap_;
		NameServiceRoot*  proot_;
};


#endif
