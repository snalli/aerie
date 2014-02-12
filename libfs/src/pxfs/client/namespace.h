#ifndef _NAMESPACE_H_AGT127
#define _NAMESPACE_H_AGT127

#include <sys/types.h>
#include "rpc/rpc.h"
#include "pxfs/client/mpinode.h"
//#include "pxfs/client/sb.h"
#include <google/dense_hash_map>
//#include "pxfs/client/cache.h"

namespace client {
class Session;

class NameSpace {
/*
#ifdef CONFIG_CACHE
	struct Dentry {
		Dentry () 
			: self(NULL),
			  parent(NULL)
		{ }

		Dentry (void *self_, void *parent_)
			: self(self_),
			  parent(parent_)
		{ }
		void *self;
		void *parent;
	};
#endif
*/
public:
	NameSpace(const char*);
	int Rename(Session* session, const char *oldpath, const char* newpath);
	int Link(Session* session, const char* oldpath, const char* newpath);
	int Unlink(Session* session, const char* pathname);
	int Namei(Session* session, const char* path, lock_protocol::Mode lock_mode, Inode**);
	int namei_sans_locks(Session* session, const char* path, lock_protocol::Mode lock_mode, Inode**);
	int Nameiparent(Session* session, const char* path, lock_protocol::Mode lock_mode, char* name, Inode**);
	int Mount(Session* session, const char*, SuperBlock*);
	int Unmount(Session* session, char*);
	int Init(Session* session);
	int SetCurWrkDir(Session* session, const char* path);


#ifdef CONFIG_CACHE
	osd::common::Cache *cache;
        osd::common::Cache *cache_arr[N_CACHE];

	void SetCache(osd::common::Cache *val);
	int Insert(const char* name, void*, void *);
	int Lookup(const char* name, void**, void **);
	int Erase(const char* name, void*, void *);
	void Prefetch(Session *session);
	struct list_item {
                void *data;
                struct list_item *next;
        };

#endif
/*
	typedef google::dense_hash_map<std::string, Dentry> DentryCache;
	unsigned long 		succEntry, failEntry;
	unsigned long 		cacheHit, cacheMiss;
	unsigned long		cacheLookup;
*/
private:
	int LockInodeReverse(Session* session, Inode* inode, lock_protocol::Mode lock_mode);
	int Namex(Session* session, const char* path, lock_protocol::Mode lock_mode, bool nameiparent, char* name, Inode**);
	int namex_sans_locks(Session* session, const char* path, lock_protocol::Mode lock_mode, bool nameiparent, char* name, Inode**);

	char         namespace_name_[128];
	MPInode*     root_;
	Inode*       cwd_;
/*
	#ifdef CONFIG_CACHE 
		DentryCache  dentries_;
		int 		DENTRY_SIZE;
		pthread_rwlock_t   rwlock;
		pthread_spinlock_t spinlock;
		int pshared;
	#endif
*/
};

} // namespace client

#endif
