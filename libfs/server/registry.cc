#include "registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <cstring>
#include "common/uthash.h"


Registry::Registry():
	registry_root_(NULL)
{
	int ret; 
	ret = pthread_mutex_init(&mutex_, NULL);
	assert(ret == 0);
}


int Registry::Init()
{
	pheap_ = new PHeap();

	pheap_->Open("registry.pheap", 1024*1024, sizeof(*registry_root_), 0, 0);

	registry_root_ = (RegistryRoot*) pheap_->get_root();
}


int Registry::Lookup(const char *name, void **ptr)
{
	REntry* rentry;
	int     i;
	
	pthread_mutex_lock(&mutex_);
	HASH_FIND_STR(registry_root_->entry_array_, name, rentry);
	if (!rentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}

	if (ptr) {
		*ptr = rentry->ptr;
	}
	pthread_mutex_unlock(&mutex_);
	return 0;
}


int Registry::Add(const char *name, void *ptr)
{
	REntry* rentry;
	int     i;
	
	assert(strlen(name)<128);

	if (!ptr) {
		return -1;
	}

	pthread_mutex_lock(&mutex_);
	HASH_FIND_STR(registry_root_->entry_array_, name, rentry);
	if (rentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}

	pheap_->Alloc(sizeof(*rentry), (void**) &rentry);
	if (!rentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	strcpy(rentry->name, name);
	rentry->ptr = ptr;
	HASH_ADD_STR(registry_root_->entry_array_, name, rentry);
	pthread_mutex_unlock(&mutex_);
	return 0;
}


int Registry::Remove(const char *name)
{
	REntry* rentry;
	int     i;

	pthread_mutex_lock(&mutex_);
	HASH_FIND_STR(registry_root_->entry_array_, name, rentry);
	if (!rentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	HASH_DEL(registry_root_->entry_array_, rentry);
	pheap_->Free((void*) rentry, sizeof(*rentry));
	pthread_mutex_unlock(&mutex_);

	return 0;
}
