#include "nameservice.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <cstring>
#include "common/uthash.h"


NameService::NameService():
	proot_(NULL)
{
	int ret; 
	ret = pthread_mutex_init(&mutex_, NULL);
	assert(ret == 0);
}


int NameService::Init()
{
	pheap_ = new PHeap();

	pheap_->Open("nameservice.pheap", 1024*1024, sizeof(*proot_), 0, 0);

	proot_ = (NameServiceRoot*) pheap_->get_root();
}


int NameService::fsck()
{
#if 0
	DEntry *dentry;
	DEntry *tmp;

	HASH_ITER(hh, dentry_array_, dentry, tmp) {
		//if (dentry->inode && ((dentry->inode->size % 2) == 1) ) {
		//	goto err;
		//}
		if (dentry->inode && (dentry->inode->size > dentry->inode->maxsize )) {
			printf("size=%d\n", dentry->inode->size );
			printf("maxsize=%d\n", dentry->inode->maxsize );
			goto err;
		}
		if (dentry->inode && (dentry->inode->maxsize % 2 != 0) ) {
			goto err;
		}
	}
	return 0;
err:
	return -1;
#endif	
}


int NameService::Lookup(const char *name, void **ptr)
{
	NEntry* nentry;
	int     i;
	
	pthread_mutex_lock(&mutex_);
	HASH_FIND_STR(proot_->entry_array_, name, nentry);
	if (!nentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}

	if (ptr) {
		*ptr = nentry->ptr_;
	}
	pthread_mutex_unlock(&mutex_);
	return 0;
}


int NameService::Link(const char *name, void *ptr)
{
	NEntry* nentry;
	int     i;
	
	assert(strlen(name)<128);

	if (!ptr) {
		return -1;
	}

	pthread_mutex_lock(&mutex_);
	HASH_FIND_STR(proot_->entry_array_, name, nentry);
	if (!nentry) {
		pheap_->Alloc(sizeof(*nentry), (void**) &nentry);
		if (!nentry) {
			pthread_mutex_unlock(&mutex_);
			return -1;
		}
		strcpy(nentry->name_, name);
		nentry->ptr_ = ptr;
		HASH_ADD_STR(proot_->entry_array_, name_, nentry);
		nentry->refcount_ = 1;
	} else {
		nentry->refcount_++;
	}
	pthread_mutex_unlock(&mutex_);
	return 0;
}


int NameService::Unlink(const char *name)
{
	NEntry* nentry;
	int     i;

	pthread_mutex_lock(&mutex_);
	HASH_FIND_STR(proot_->entry_array_, name, nentry);
	if (!nentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	if (--nentry->refcount_<1) {
		HASH_DEL(proot_->entry_array_, nentry);
		pheap_->Free((void*) nentry, sizeof(*nentry));
	}	
	pthread_mutex_unlock(&mutex_);

	return 0;
}
