#ifndef _NAMESERVICE_H_IKL111
#define _NAMESERVICE_H_IKL111

#include <pthread.h>
#include "common/uthash.h"

typedef void inode_t;
typedef struct DEntry DEntry;

struct DEntry {
	char           name[128];
	inode_t*       inode;
	UT_hash_handle hh;
};

class NameService {
	public:
		NameService();
		int fsck();
		int Lookup(const char *, inode_t **inode);
		int Link(const char *, inode_t *inode);
		int Remove(const char *);
	private:
		DEntry*         dentry_array_;
		pthread_mutex_t mutex_;
};


#endif
