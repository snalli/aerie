#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <cstring>
#include "common/uthash.h"
#include "nameservice.h"

//FIXME: Use file handles (ala NFS) instead of inodes: <fsid, ino, gen>
//FIXME: Do we need to keep inode generation numbers? NFS does 

NameService::NameService():
	dentry_array_(NULL)
{
	int ret; 
	ret = pthread_mutex_init(&mutex_, NULL);
	assert(ret == 0);
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

/*
int NameService::FindInode(inode_t *inode)
{
	DEntry* dentry;
	DEntry* tmp;

	HASH_ITER(hh, dentry_array_, dentry, tmp) {
		if (dentry->inode == inode) {
			return 0;
		}
	}
	return -1;
}
*/

int NameService::Lookup(const char *name, inode_t **inode)
{
	DEntry* dentry;
	int     i;

	pthread_mutex_lock(&mutex_);
	HASH_FIND_STR(dentry_array_, name, dentry);
	if (!dentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}

	if (inode) {
		*inode = dentry->inode;
	}
	pthread_mutex_unlock(&mutex_);
	return 0;
}

int NameService::Link(const char *name, inode_t *inode)
{
	DEntry* dentry;
	int     i;
	
	assert(strlen(name)<128);

	if (!inode) {
		return -1;
	}

	pthread_mutex_lock(&mutex_);
	HASH_FIND_STR(dentry_array_, name, dentry);
	if (dentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}

	dentry = new DEntry;
	if (!dentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	strcpy(dentry->name, name);
	dentry->inode = inode;
	HASH_ADD_STR(dentry_array_, name, dentry);
	//HASH_ADD_KEYPTR( hh, dentry_array_, dentry->path, strlen(dentry->path), dentry);
	pthread_mutex_unlock(&mutex_);
	return 0;
}

int NameService::Remove(const char *name)
{
	DEntry* dentry;
	int     i;

	pthread_mutex_lock(&mutex_);
	HASH_FIND_STR(dentry_array_, name, dentry);
	if (!dentry) {
		pthread_mutex_unlock(&mutex_);
		return -1;
	}
	HASH_DEL(dentry_array_, dentry);
	delete dentry;
	pthread_mutex_unlock(&mutex_);

	return 0;
}
