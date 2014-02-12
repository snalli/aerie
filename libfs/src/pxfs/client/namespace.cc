
#include "pxfs/client/namespace.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <stack>
#include <typeinfo>
#include "common/util.h"
#include "common/hrtime.h"
#include "osd/main/client/stm.h"
#include "pxfs/client/client_i.h"
//#include "pxfs/mfs/client/file_inode.h"
#include "pxfs/client/fsomgr.h"
//#include "pxfs/client/mpinode.h"
//#include "pxfs/client/inode.h"
//#include "pxfs/client/sb.h"
#include "pxfs/client/session.h"
#include "pxfs/common/publisher.h"
#include "bcs/bcs.h"
#include <string.h>
#include <libgen.h>

//TODO: directory operations update object version
//TODO: port and test rename

// LOCK PROTOCOL
// 
// read-only file:
//
// need to locally lock each inode along the path at level SL. 
// if the local lock has no parent lock then you need to 
// acquire a base SR lock on the inode.
// 
// read-write file:
//
// need to locally lock each inode except the file inode at IXSL
// if the local lock has no parent lock then you need to 
// acquire a base XR lock on the inode. 
// locally lock the file inode at XL
//

//osd::common::ObjectProxy p;
namespace client {
char *strrev(char *str)
{
      char *p1, *p2;

      if (! str || ! *str)
            return str;
      for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
      {
            *p1 ^= *p2;
            *p2 ^= *p1;
            *p1 ^= *p2;
      }
      return str;
}
NameSpace::NameSpace(const char* namespace_name)
{
	#ifdef CONFIG_CACHE
//	cache.init_cache();
/*		dentries_.set_empty_key("__EMPTY_KEY__");
		dentries_.set_deleted_key("__DELETED_KEY__");
		succEntry = 0;
		failEntry=0;
		cacheHit=0;
		cacheMiss=0;
		cacheLookup=0;
		DENTRY_SIZE = 1000;
		pthread_rwlock_init(&rwlock, NULL);
		pthread_spin_init(&spinlock, pshared);
		printf("\n***************************************************\n");
		printf("\nPID : %d \nInitializing NameSpace...\nCache Hits : %d \nCache Miss : %d \nCache Accs : %d \n",getpid(), cacheHit, cacheMiss, cacheLookup);
		printf("Cache Inst : %d\n", succEntry);
		printf("\n***************************************************\n");
*/	
	 for(int n_cache = 0;n_cache < N_CACHE; ++n_cache)
        {
                cache_arr[n_cache] = new osd::common::Cache::Cache(n_cache);
        }

	#endif
	strcpy(namespace_name_, namespace_name);
}


const int NAMESIZ = 128;
const int KERNEL_SUPERBLOCK = 0x1;

//! Copy the next path element from path into name.
//! Return a pointer to the element following the copied one.
//! The returned path has no leading slashes,
//! so the caller can check *path=='\0' to see if the name is the last one.
//! If no name to remove, return 0.
//
//! Examples:
//!   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//!   skipelem("///a//bb", name) = "bb", setting name = "a"
//!   skipelem("a", name) = "", setting name = "a"
//!   skipelem("", name) = skipelem("////", name) = 0
//!
static char*
SkipElem(char *path, char *name)
{
	char* s;
	int   len;

	while(*path == '/') {
		path++;
	}	
	if(*path == 0) {
		return 0;
	}	
	s = path;
	while(*path != '/' && *path != 0) {
		path++;
	}
	len = path - s;
	if(len >= NAMESIZ) {
		memmove(name, s, NAMESIZ);
	} else {
		memmove(name, s, len);
		name[len] = 0;
	}
	while(*path == '/') {
		path++;
	}	
	return path;
}




int
NameSpace::Init(Session* session) 
{
	//printf("\n Sanketh : Inside Namespace::Init... \n");
	root_ = new MPInode;
	
//	printf("\nInitializing NameSpace...");
	//printf("Init : root_=%p \n", root_);
	cwd_ = root_;
	//printf("\n Return from NameSpace::Mount : %d\n",Mount(session, "/", (SuperBlock*) KERNEL_SUPERBLOCK));
	return E_SUCCESS;
}


#ifdef CONFIG_CACHE
void
NameSpace::SetCache(osd::common::Cache *val)
{
	cache = val;
}
int
NameSpace::Lookup(const char* name, void** ip, void**dip)
{

	 int index = strlen(name)%N_CACHE;

        return cache_arr[index]->lookup_cache(name, ip, dip);
}
int
NameSpace::Insert(const char* name, void* ip, void *dip)
{
	int index = strlen(name)%N_CACHE;
        return cache_arr[index]->insert_cache(name, ip, dip);

}
int
NameSpace::Erase(const char* name, void* ip, void* dip)
{
  int index = strlen(name)%N_CACHE;

        return cache_arr[index]->erase_cache(name);
}
void
NameSpace::Prefetch(Session *session)
{

	s_log("[%ld] NameSpace::%s Start",s_tid, __func__);
	printf("[%ld] NameSpace::%s Prefetching...\n",s_tid, __func__);
        Inode *inode;
        Inode *tmp_inode;
        Inode *ip;

        inode = (Inode *)(root_->return_pxfs_inode());
        struct list_item inode_list_head;
        struct list_item *ptr;

        inode_list_head.data = (void *) &inode_list_head;
        inode_list_head.next = NULL;

        strcpy(inode->self_name, root_->self_name);
        strcat(inode->self_name, "pxfs");
        inode->parent = root_;

        Insert(inode->self_name, inode, inode->parent);

        inode->return_dentry(session, (void *)&inode_list_head);

        ptr = inode_list_head.next;

        while(ptr)
        {
                ip = (Inode *)ptr->data;
                Insert(ip->self_name, ip, ip->parent);
		//printf("[%ld] name=%s \n", s_tid, ip->self_name);	
                ip->return_dentry(session, (void *)&inode_list_head);
                ptr = ptr->next;
        }
        s_log("[%ld] NameSpace::%s End",s_tid, __func__);

}
#endif



int
NameSpace::Mount(Session* session, const char* const_path, SuperBlock* sb)
{
	//printf("\n Inside NameSpace::Mount...\n");
	char   name[128];
	Inode* inode;
	Inode* inode_next;
	int    ret;
	char*  path = const_cast<char*>(const_path);

	inode = root_;
	while((path = SkipElem(path, name)) != 0) {
		//printf("\n Sanketh : Calling MPInode::Lookup from NameSpace::Mount... \n");
		if ((ret = inode->Lookup(session, name, 0, (Inode**) &inode_next)) < 0) {
			//printf("\n Sanketh : Return from MPInode::Lookup failed : %d \n", ret);
			if (typeid(*inode) != typeid(client::MPInode)) {
				// cannot mount a filesystem on non-mount-point pseudo-inode  
				////////printf("\n Sanketh : Inside MPInode::Lookup. Inode types don't match\n");
				return -1;
			}
			if (*path == '\0') {
				// reached end of path -- mount superblock
				Inode* root_inode = sb->RootInode();
				//printf("\n Sanketh 1 : Calling MPInode::Link...\n");
				assert(inode->Link(session, name, root_inode, false) == 0);	
				if (root_inode->Link(session, "..", inode, false) != 0) {
					return -1; // superblock already mounted
				}
				break;
			} else {
				// create mount point component
				inode_next = new MPInode();
				//printf("\n Sanketh 2 : Calling MPInode::Link...\n");
				assert(inode->Link(session, name, inode_next, false) == 0);	
				//printf("\n Sanketh 3 : Calling MPInode::Link...\n");
				assert(inode_next->Link(session, ".", inode_next, false) == 0);	
				//printf("\n Sanketh 4 : Calling MPInode::Link...\n");
				assert(inode_next->Link(session, "..", inode, false) == 0);	
			}
		}
		inode = inode_next;
	}

	//printf("\n Return from NameSpace::Mount SUCCESS ! \n");
	return 0;
}


int
NameSpace::Unmount(Session* session, char* name)
{
	return 0;
}


// caller does not hold any lock 
// 
// it relies on a read-only optimistic memory transaction to atomically 
// traverse the chain up to the root and then acquire locks down the path 
// until it finds the inode
int
NameSpace::LockInodeReverse(Session* session, Inode* inode, lock_protocol::Mode lock_mode)
{
	std::vector<Inode*> inode_chain;   // treated as a stack
	std::vector<Inode*> locked_inodes; 
	Inode*              tmp_inode;
	Inode*              parent_inode;
	int                 ret;
	int                 retries = 0;

	STM_BEGIN()
		// the transaction does not automatically release any acquired locks 
		// so we need to do this manually
		inode_chain.clear();
		for (std::vector<Inode*>::iterator it = locked_inodes.begin();
		     it != locked_inodes.end();
			 it++)
		{
			(*it)->Unlock(session);
		}
		locked_inodes.clear();
		// optimistically find all the inodes that appear in the path that begins
		// from the current inode up to the root inode
		for (tmp_inode = inode; tmp_inode->oid().u64() != 0; tmp_inode = parent_inode) {
			inode_chain.push_back(tmp_inode);
			tmp_inode->xOpenRO(session);
			ret = tmp_inode->xLookup(session, "..", 0, &parent_inode);
			if (ret != E_SUCCESS) {
			
			}
		if (retries++ > 5) {
			assert(0);
		}

			STM_ABORT_IF_INVALID() // read-set consistency
		}
		// acquire hierarchical locks on the inodes in the reverse order they 
		// appear in the list (i.e. root to leaf)
		parent_inode = NULL;
		for (std::vector<Inode*>::iterator it = inode_chain.end()-1;
		     it >= inode_chain.begin();
			 it--)
		{
			if (*it == inode) {
				if (parent_inode) {
					assert((*it)->Lock(session, parent_inode, lock_mode) == 0);
				} else {
					assert(*it == root_);
					assert((*it)->Lock(session, lock_mode) == 0);
				}
			} else {
				if (parent_inode) {
					assert((*it)->Lock(session, parent_inode, lock_protocol::Mode::IX) == 0);
				} else {
					assert((*it)->Lock(session, lock_protocol::Mode::IX) == 0);
				}
			}
			parent_inode = *it;
		}
	STM_END()

	// now release all the locks except the lock on the inode we were asked to lock
	for (std::vector<Inode*>::iterator it = locked_inodes.begin();
		 it != locked_inodes.end();
		 it++)
	{
		if (*it != inode) {
			(*it)->Unlock(session);
		}
		}
	return E_SUCCESS;
}


// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Inode is locked at mode lock_mode
//
//The last 3 lines in english : If the path name is absolute,
//get the inode for the parent and put the rest of the path name
//in "name" and repeat this procedure.
//
int
NameSpace::Namex(Session* session, const char *cpath, lock_protocol::Mode lock_mode, 
                 bool nameiparent, char* name, Inode** inodep)
{
	char*       path;
	Inode*      inode;
	Inode*      inode_next;
	int         ret;
	char*       old_name;
	int lcount = 0;
	bool smart_lookup;

retry_namex:
	path = const_cast<char*>(cpath);
	if (!path) {
		return -E_INVAL;
	}

	DBG_LOG(DBG_INFO, DBG_MODULE(client_name), "Namex: %s\n", path);

/*******************************************************************************
 * Sanketh's code starts here !
 * This section looks up the cache
 */	
#ifdef CONFIG_CACHE

	char	    dup_path[128], residue[128], tmp[128], tmp01[128];
	char *dirc, *basec, *bname, *dname;
	Inode*      parent;
int itr = 0;
	strcpy(dup_path, "/");
		dirc = strdup(path);
		basec = strdup(path);
		dname = dirname(dirc);
		bname = basename(basec);
	smart_lookup = false;
	char look_for[128], *helper;
	strcpy(residue,"/");

	if (nameiparent) {
		if ( strcmp(dname,"/")  == 0) {
			inode = root_;
			inode->Get();
			inode->Lock(session, lock_mode);
			strcpy(name,bname);
			goto done;
		} else {
			if(Lookup(dname, (void **) &inode, (void **) &parent)) {
				inode->Get();
				inode->Lock(session, parent, lock_mode);
				strcpy(name,bname);
				goto done;
			} else {
				smart_lookup = true;
				dirc = strdup(dname);
				basec = strdup(dname);
				strcat(residue, strrev(bname));
				strcat(residue,"/");
				strcpy(look_for, dirname(dirc));
				strcpy(tmp, basename(basec));
				strcat(residue, strrev(tmp));
				//s_log("[%ld] (1) NameSpace::%s look_for=%s",s_tid, __func__, look_for);
				//s_log("[%ld] (1) NameSpace::%s residue=%s",s_tid, __func__, residue);
			}
		}
	} else {
		if ( strcmp(bname,"/")  == 0) {
			inode = root_;
			inode->Get();
			inode->Lock(session, lock_protocol::Mode::IXSL);
			goto done;
		} else {
			if(Lookup(path, (void **)&inode, (void **)&parent)) {
				inode->Get();
				inode->Lock(session, parent, lock_mode);
				goto done;
			} else {
				smart_lookup = true;
				dirc = strdup(path);
				basec = strdup(path);
				strcpy(look_for, dirname(dirc));
				strcpy(tmp, basename(basec));
				strcat(residue, strrev(tmp));
				//s_log("[%ld] (2) NameSpace::%s look_for=%s",s_tid, __func__, look_for);
				//s_log("[%ld] (2) NameSpace::%s residue=%s",s_tid, __func__, residue);
			}
		}
	}
	
		
	if (smart_lookup) {
#ifdef SMART_LKUP
	
		while(1) {

			if(Lookup(look_for, (void **)&inode, (void **)&parent)){
				inode->Get();
				inode->Lock(session, parent, lock_mode);

				basec = strdup(residue);
				dirc = strdup(residue);

				strcpy(tmp, basename(basec));
				strcpy(name,strrev(tmp));
				//s_log("[%ld] NameSpace::%s name=%s",s_tid, __func__, name);
				// next component

				strcat(look_for, "/");
				strcat(look_for, name);
				strcpy(dup_path, look_for);
				//s_log("[%ld] NameSpace::%s dup_path=%s",s_tid, __func__, dup_path);
				// path name of next component
				
				dname = dirname(dirc);
				if(strcmp(dname, "/") == 0){
					path = tmp01;	
					strcpy(path,"\0");
					//residue
				} else {
					strcpy(tmp, dname);
					strrev(tmp);
					path = tmp01;	
					strcpy(path, tmp) ;
					// residue
				}
				//s_log("[%ld] NameSpace::%s path=%s",s_tid, __func__, path);
				goto resume_normal;
			}
			strcpy(tmp, look_for);
			strcat(residue,"/");
			strcpy(tmp01, basename(tmp));
			strcat(residue,strrev(tmp01));
			strcpy(look_for, dirname(tmp));
			if(strcmp(look_for, "/") == 0){
				goto traditional;
			}
		}
#endif		

	}
	// Take care of locks and refcounts before returning the inode pointer !
#endif
/*******************************************************************************/	
/*
 * INSIGHT : 
 * Insert in cache after every successful inode lookup
 * Take care of string names
 * You'll have to concern yourself only with absolute pathnames
 * as file bench uses only those
 */


/*
 * Insight : If you have made it this far, * it means that the look-up for the 
 * ultimate inode has failed. We now proceed to path name resolution. 
 */
	// boundary condition: get the lock on the first inode
	// find whether absolute or relative path
traditional :
	if (*path == '/') {
		inode = root_; // The root inode, as always, is "well known" !
			       // root_ is of type MPInode. Inode is an abstract class. 
			       // You cannot instantiate an abstract class, because it has virtual functions which have not yet been defined.
			       // The task of defining the virtual functions is delegated to child classes.
			       // In this case, Inode::Lock is a virtual function which needs to be defined before use.
			       // MPInode does this, and hence we are able to use Inode::Lock.
				// DO ME : Verify that we really touch MPInode::Lock
		//printf("Namex 1 : inode=%p (ino=%lu), name=%s\n", inode, inode->ino(), name);
		path = SkipElem(path, name);
		/*
		 * Insight : The knowledge of the workload helps here. 
		 * Since I know that file bench uses only absolute names,
		 * I won't bother inserting my cache modifications in the 
		 * else branch. But for generality, you should.
		 */
		#ifdef CONFIG_CACHE
			strcat(dup_path, name);
		#endif
		// insight : I think basename and dirname use the kernel !! Hence skipElem.
		// See if you can use it too, else use basename and dirname for now and then 
		// replace it with your implementation.
		// SkipElem("/a/b/c",name) = b/c, name = a
		//printf("Namex 1.1 : inode=%p (ino=%lu), name=%s\n", inode, inode->ino(), name);
		if (nameiparent && path != 0 && *path == '\0') { // If we are here, it means the filename was just "/" !!!!!!
			inode->Lock(session, lock_mode); // Check pxfs/mfs/client/dir_inode.cc
							 // 
			inode->Get(); // Increments the ref_cnt of root_
			goto done;
		} else {
			inode->Lock(session, lock_protocol::Mode::IXSL);
			inode->Get();
		}
	} else {
		inode = cwd_;
		path = SkipElem(path, name);
		if (nameiparent && path != 0 && *path == '\0') {
			if (inode->Lock(session, lock_mode) != 0) {
				LockInodeReverse(session, inode, lock_mode);
			}
			inode->Get();
			goto done;
		} else {
			if (inode->Lock(session, lock_mode) != 0) {
				LockInodeReverse(session, inode, lock_protocol::Mode::IXSL); // insight : Locking all the ancestors in SIX mode
			}
			inode->Get();
		}
	}

	// consume the rest of the path by acquiring a lock on each inode in a spider
	// locking fashion. spider locking is necessary because acquiring an
	// hierarchical lock on an inode requires having the parent locked.
	// if we encounter a .. then we move up the hierarchy but we don't do spider 
	// locking. however the client may no longer own the lock on the .. if another
	// client asked the lock. in this case we need to boostrap the lock by
	// acquiring locks along the chain from the root to the inode ..
resume_normal:
	while (path != 0) { // Traverse the pathname to get to target file
		// DO ME : Before the inode lookup, you can have a cache look up !
		// If that fails, then you can proceed with the inode lookup
/*
				s_log("[%ld] NameSpace::%s ======================================",s_tid, __func__);
				s_log("[%ld] NameSpace::%s itr=%d",s_tid, __func__, itr++);
				s_log("[%ld] NameSpace::%s name=%s",s_tid, __func__, name);
				s_log("[%ld] NameSpace::%s dup_path=%s",s_tid, __func__, dup_path);
				s_log("[%ld] NameSpace::%s path=%s",s_tid, __func__, path);
				s_log("[%ld] NameSpace::%s inode=%p",s_tid, __func__, inode);
				s_log("[%ld] NameSpace::%s ======================================",s_tid, __func__);
*/
		if ((ret = inode->Lookup(session, name, 0, &inode_next)) < 0) {
			inode->Put();
			inode->Unlock(session);
			//	s_log("[%ld] NameSpace::%s Lookup failed",s_tid, __func__);
			return ret;
		}
			//	s_log("[%ld] NameSpace::%s Lookup success",s_tid, __func__);

		#ifdef CONFIG_CACHE
			Insert(dup_path, inode_next, inode);
		#endif
		old_name = name; 
		path = SkipElem(path, name);
			//	s_log("[%ld] NameSpace::%s chk_pt 01 path=%s name=%s",s_tid, __func__, path, name);

		#ifdef CONFIG_CACHE
			strcat(dup_path,"/");
			strcat(dup_path, name);
			//	s_log("[%ld] NameSpace::%s chk_pt 02 dup_path=%s name=%s",s_tid, __func__, dup_path, name);
		#endif


		if (nameiparent && path != 0 && *path == '\0') {
			if (str_is_dot(old_name) == 2) {
				// encountered a ..
				inode->Unlock(session);
				assert(inode_next->Lock(session, lock_mode) == E_SUCCESS);
			//	s_log("[%ld] NameSpace::%s chk_pt 03",s_tid, __func__);
			} else {
				// spider locking
				assert(inode_next->Lock(session, inode, lock_mode) == E_SUCCESS); 
				inode->Unlock(session);
			//	s_log("[%ld] NameSpace::%s chk_pt 04",s_tid, __func__);
			}
			inode->Put();
			inode = inode_next;
			//	s_log("[%ld] NameSpace::%s chk_pt 05",s_tid, __func__);
			goto done;
		} else {
			if (str_is_dot(old_name) == 2) {
				// encountered a ..
				// Cannot do hierarchical locking on the reverse order (i.e. acquire 
				// parent under child) so we try to acquire the cached lock on .. 
				// by not providing a parent hint for ..
				// If this fails then we redo locking beginning from the root
				inode->Unlock(session);
			//	s_log("[%ld] NameSpace::%s chk_pt 06",s_tid, __func__);
				if (inode_next->Lock(session, lock_protocol::Mode::IXSL) != E_SUCCESS) {
					LockInodeReverse(session, inode_next, lock_protocol::Mode::IXSL);
			//	s_log("[%ld] NameSpace::%s chk_pt 07",s_tid, __func__);
				}
			//	s_log("[%ld] NameSpace::%s chk_pt 08",s_tid, __func__);
			} else {
				if (path == 0) {
					// last path componenent
					if (inode_next->Lock(session, inode, lock_mode) != E_SUCCESS) {
						DBG_LOG(DBG_INFO, DBG_MODULE(client_name), "ABORT\n");
						inode->Unlock(session);
						inode->Put();
			//	s_log("[%ld] NameSpace::%s chk_pt 09",s_tid, __func__);
						goto retry_namex;
					}
			//	s_log("[%ld] NameSpace::%s chk_pt 10",s_tid, __func__);
				} else {
					if (inode_next->Lock(session, inode, lock_protocol::Mode::IXSL) != E_SUCCESS) {
						DBG_LOG(DBG_INFO, DBG_MODULE(client_name), "ABORT\n");
						inode->Unlock(session);
						inode->Put();
			//	s_log("[%ld] NameSpace::%s chk_pt 11",s_tid, __func__);
						goto retry_namex;
					}
			//	s_log("[%ld] NameSpace::%s chk_pt 12",s_tid, __func__);
				}
				inode->Unlock(session);
			//	s_log("[%ld] NameSpace::%s chk_pt 13",s_tid, __func__);
			}
			inode->Put();
			inode = inode_next;
			//	s_log("[%ld] NameSpace::%s chk_pt 15",s_tid, __func__);
		}
	}
done:
			//	s_log("[%ld] NameSpace::%s chk_pt 16",s_tid, __func__);
	*inodep = inode; // insight : If the path name is "/a/b/c/" do you return with the inode of "b" ????
			// insight : Yes, we return the inode of b 
	return 0;
}

int
NameSpace::namex_sans_locks(Session* session, const char *cpath, lock_protocol::Mode lock_mode, 
                 bool nameiparent, char* name, Inode** inodep)
{
	char*       path;
	Inode*      inode;
	Inode*      inode_next;
	int         ret;
	char*       old_name;
	int lcount = 0;
	bool smart_lookup;

retry_namex:
	path = const_cast<char*>(cpath);
	if (!path) {
		return -E_INVAL;
	}


/*******************************************************************************
 * Sanketh's code starts here !
 * This section looks up the cache
 */	
#ifdef CONFIG_CACHE

	char	    dup_path[128], residue[128], tmp[128], tmp01[128];
	char *dirc, *basec, *bname, *dname;
	Inode*      parent;
int itr = 0;
	strcpy(dup_path, "/");
		dirc = strdup(path);
		basec = strdup(path);
		dname = dirname(dirc);
		bname = basename(basec);
	smart_lookup = false;
	char look_for[128], *helper;
	strcpy(residue,"/");

	if (nameiparent) {
		if ( strcmp(dname,"/")  == 0) {
			inode = root_;
			inode->Get();
			strcpy(name,bname);
			goto done;
		} else {
			if(Lookup(dname, (void **) &inode, (void **) &parent)) {
				inode->Get();
				strcpy(name,bname);
				goto done;
			} else {
				smart_lookup = true;
				dirc = strdup(dname);
				basec = strdup(dname);
				strcat(residue, strrev(bname));
				strcat(residue,"/");
				strcpy(look_for, dirname(dirc));
				strcpy(tmp, basename(basec));
				strcat(residue, strrev(tmp));
			}
		}
	} else {
		if ( strcmp(bname,"/")  == 0) {
			inode = root_;
			inode->Get();
			goto done;
		} else {
			if(Lookup(path, (void **)&inode, (void **)&parent)) {
				inode->Get();
				goto done;
			} else {
				smart_lookup = true;
				dirc = strdup(path);
				basec = strdup(path);
				strcpy(look_for, dirname(dirc));
				strcpy(tmp, basename(basec));
				strcat(residue, strrev(tmp));
			}
		}
	}
	
		
	if (smart_lookup) {
#ifdef SMART_LKUP
	
		while(1) {

			if(Lookup(look_for, (void **)&inode, (void **)&parent)){
				inode->Get();

				basec = strdup(residue);
				dirc = strdup(residue);

				strcpy(tmp, basename(basec));
				strcpy(name,strrev(tmp));
				// next component

				strcat(look_for, "/");
				strcat(look_for, name);
				strcpy(dup_path, look_for);
				// path name of next component
				
				dname = dirname(dirc);
				if(strcmp(dname, "/") == 0){
					path = tmp01;	
					strcpy(path,"\0");
					//residue
				} else {
					strcpy(tmp, dname);
					strrev(tmp);
					path = tmp01;	
					strcpy(path, tmp) ;
					// residue
				}
				goto resume_normal;
			}
			strcpy(tmp, look_for);
			strcat(residue,"/");
			strcpy(tmp01, basename(tmp));
			strcat(residue,strrev(tmp01));
			strcpy(look_for, dirname(tmp));
			if(strcmp(look_for, "/") == 0){
				goto traditional;
			}
		}
#endif		

	}
	// Take care of locks and refcounts before returning the inode pointer !
#endif
/*******************************************************************************/	
/*
 * INSIGHT : 
 * Insert in cache after every successful inode lookup
 * Take care of string names
 * You'll have to concern yourself only with absolute pathnames
 * as file bench uses only those
 */


/*
 * Insight : If you have made it this far, * it means that the look-up for the 
 * ultimate inode has failed. We now proceed to path name resolution. 
 */
	// boundary condition: get the lock on the first inode
	// find whether absolute or relative path
traditional :
	if (*path == '/') {
		inode = root_; // The root inode, as always, is "well known" !
			       // root_ is of type MPInode. Inode is an abstract class. 
			       // You cannot instantiate an abstract class, because it has virtual functions which have not yet been defined.
			       // The task of defining the virtual functions is delegated to child classes.
			       // In this case, Inode::Lock is a virtual function which needs to be defined before use.
			       // MPInode does this, and hence we are able to use Inode::Lock.
				// DO ME : Verify that we really touch MPInode::Lock
		path = SkipElem(path, name);
		/*
		 * Insight : The knowledge of the workload helps here. 
		 * Since I know that file bench uses only absolute names,
		 * I won't bother inserting my cache modifications in the 
		 * else branch. But for generality, you should.
		 */
		#ifdef CONFIG_CACHE
			strcat(dup_path, name);
		#endif
		// insight : I think basename and dirname use the kernel !! Hence skipElem.
		// See if you can use it too, else use basename and dirname for now and then 
		// replace it with your implementation.
		// SkipElem("/a/b/c",name) = b/c, name = a
		if (nameiparent && path != 0 && *path == '\0') { // If we are here, it means the filename was just "/" !!!!!!
			inode->Get(); // Increments the ref_cnt of root_
			goto done;
		} else {
			inode->Get();
		}
	} else {
		inode = cwd_;
		path = SkipElem(path, name);
		if (nameiparent && path != 0 && *path == '\0') {
			if (inode->Lock(session, lock_mode) != 0) {
				LockInodeReverse(session, inode, lock_mode);
			}
			inode->Get();
			goto done;
		} else {
			if (inode->Lock(session, lock_mode) != 0) {
				LockInodeReverse(session, inode, lock_protocol::Mode::IXSL); // insight : Locking all the ancestors in SIX mode
			}
			inode->Get();
		}
	}

	// consume the rest of the path by acquiring a lock on each inode in a spider
	// locking fashion. spider locking is necessary because acquiring an
	// hierarchical lock on an inode requires having the parent locked.
	// if we encounter a .. then we move up the hierarchy but we don't do spider 
	// locking. however the client may no longer own the lock on the .. if another
	// client asked the lock. in this case we need to boostrap the lock by
	// acquiring locks along the chain from the root to the inode ..
resume_normal:
	while (path != 0) { // Traverse the pathname to get to target file
		// DO ME : Before the inode lookup, you can have a cache look up !
		// If that fails, then you can proceed with the inode lookup

		if ((ret = inode->Lookup(session, name, 0, &inode_next)) < 0) {
			inode->Put();
			return ret;
		}

		#ifdef CONFIG_CACHE
			Insert(dup_path, inode_next, inode);
		#endif
		old_name = name; 
		path = SkipElem(path, name);
		#ifdef CONFIG_CACHE
			strcat(dup_path,"/");
			strcat(dup_path, name);
		#endif

		if (nameiparent && path != 0 && *path == '\0') {
                        inode->Put();
                        inode = inode_next;
                        goto done;
                } else {
                        inode->Put();
                        inode = inode_next;
                }

	}

/*
		if (nameiparent && path != 0 && *path == '\0') {
			if (str_is_dot(old_name) == 2) {
				// encountered a ..
				inode->Unlock(session);
				assert(inode_next->Lock(session, lock_mode) == E_SUCCESS);
			} else {
				// spider locking
				assert(inode_next->Lock(session, inode, lock_mode) == E_SUCCESS); 
				inode->Unlock(session);
			}
			inode->Put();
			inode = inode_next;
			goto done;
		} else {
			if (str_is_dot(old_name) == 2) {
				// encountered a ..
				// Cannot do hierarchical locking on the reverse order (i.e. acquire 
				// parent under child) so we try to acquire the cached lock on .. 
				// by not providing a parent hint for ..
				// If this fails then we redo locking beginning from the root
				inode->Unlock(session);
				if (inode_next->Lock(session, lock_protocol::Mode::IXSL) != E_SUCCESS) {
					LockInodeReverse(session, inode_next, lock_protocol::Mode::IXSL);
				}
			} else {
				if (path == 0) {
					// last path componenent
					if (inode_next->Lock(session, inode, lock_mode) != E_SUCCESS) {
						DBG_LOG(DBG_INFO, DBG_MODULE(client_name), "ABORT\n");
						inode->Unlock(session);
						inode->Put();
						goto retry_namex;
					}
				} else {
					if (inode_next->Lock(session, inode, lock_protocol::Mode::IXSL) != E_SUCCESS) {
						DBG_LOG(DBG_INFO, DBG_MODULE(client_name), "ABORT\n");
						inode->Unlock(session);
						inode->Put();
						goto retry_namex;
					}
				}
				inode->Unlock(session);
			}
			inode->Put();
			inode = inode_next;
		}
	}
*/
done:
	*inodep = inode; // insight : If the path name is "/a/b/c/" do you return with the inode of "b" ????
			// insight : Yes, we return the inode of b 
	return 0;
}


int
NameSpace::Nameiparent(Session* session, const char* path, lock_protocol::Mode lock_mode, char* name, Inode** inodep)
{
// insight : inodep is the parent inode
	int ret; 
	ret = Namex(session, path, lock_mode, true, name, inodep);
	return ret;
}


int
NameSpace::Namei(Session* session, const char* path, lock_protocol::Mode lock_mode, Inode** inodep)
{
	int  ret; 
	char name[128];

	ret = Namex(session, path, lock_mode, false, name, inodep);
	return ret;
}

int
NameSpace::namei_sans_locks(Session *session, const char* path, lock_protocol::Mode lock_mode, Inode **inodep)
{
	int ret;
	char name[128];
	
	ret = namex_sans_locks(session, path, lock_mode, false, name, inodep);
	return ret;
}

int 
NameSpace::SetCurWrkDir(Session* session, const char* path)
{
	Inode* inode;
	int    ret;

//	printf("\nChanging directory. Inside NameSpace::SetCurWrkDir...");
	DBG_LOG(DBG_INFO, DBG_MODULE(client_name), "Chdir: %s\n", path);

	if ((ret = Namei(session, path, lock_protocol::Mode::IXSL, &inode)) < 0) {
		return ret;
	}
	cwd_ = inode;
	return 0;
}


int
NameSpace::Rename(Session* session, const char *oldpath, const char* newpath)
{
	char          name_old[128];
	char          name_new[128];
	Inode*        dp_old;
	Inode*        dp_new;
	Inode*        ip;
	int           ret;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_name), "Rename: %s -> %s\n", oldpath, newpath);

grab_locks:
	// grab a recursive lock to force other clients drop any locks they acquired 
	// under the old parent (as those locks will no longer be valid after the rename)
	if ((ret = global_namespace->Nameiparent(session, oldpath, lock_protocol::Mode::XR, 
	                                         name_old, &dp_old)) < 0) 
	
{
		return ret;
	}
	if ((ret = global_namespace->Nameiparent(session, newpath, lock_protocol::Mode::XR, 
	                                         name_new, &dp_new)) < 0) 
	{
		dp_old->Put();
		dp_old->Unlock(session);
		if (ret == lock_protocol::DEADLK) {
			goto grab_locks;
		}
		return ret;
	}
	if ((ret = dp_old->Lookup(session, name_old, 0, &ip)) != E_SUCCESS) {
		dp_old->Put();
		dp_old->Unlock(session);
		dp_new->Put();
		dp_new->Unlock(session);
		return ret;
	}
	if ((ret = ip->Lock(session, dp_old, lock_protocol::Mode::XR)) != E_SUCCESS) {
		dp_old->Put();
		dp_old->Unlock(session);
		dp_new->Put();
		dp_new->Unlock(session);
		if (ret == lock_protocol::DEADLK) {
			goto grab_locks;
		}
		return ret;
	}
	session->journal()->TransactionBegin();
	
	assert(dp_new->Link(session, name_new, ip, false) == 0);
	assert(dp_old->Unlink(session, name_old) == 0);

	session->journal() << Publisher::Message::LogicalOperation::Link(dp_new->ino(), name_new, ip->ino());
	session->journal() << Publisher::Message::LogicalOperation::Unlink(dp_old->ino(), name_old);
	session->journal()->TransactionCommit();
	
	dp_old->Put();
	dp_old->Unlock(session);
	dp_new->Put();
	dp_new->Unlock(session);
	ip->Put();
	ip->Unlock(session);
	return 0;

bad:
	//FIXME: re-lock
	assert(ip->set_nlink(ip->nlink() - 1) == 0);
	return ret;
}



int
NameSpace::Link(Session* session, const char *oldpath, const char* newpath)
{
	char          name[128];
	Inode*        dp;
	Inode*        ip;
	int           ret;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_name), "Link: %s -> %s\n", oldpath, newpath);

	if ((ret = global_namespace->Namei(session, oldpath, lock_protocol::Mode::XL, 
	                                   &ip)) < 0) 
	{
		return ret;
	}
	if (ip->type() == kDirInode) {
		ip->Put();
		ip->Unlock(session);
		return -E_INVAL;
	}

	assert(ip->set_nlink(ip->nlink() + 1) == 0);
	ip->Unlock(session);
	
	if ((ret = global_namespace->Nameiparent(session, newpath, lock_protocol::Mode::XL, 
	                                         name, &dp)) < 0) 
	{
		goto bad;
	}

	assert(dp->Link(session, name, ip, false) == 0);

	session->journal() << Publisher::Message::LogicalOperation::Link(dp->ino(), name, ip->ino());
	return 0;

bad:
	//FIXME: re-lock
	assert(ip->set_nlink(ip->nlink() - 1) == 0);
	return ret;
}


int
NameSpace::Unlink(Session* session, const char *pathname)
{
        s_log("[%ld] NameSpace::%s %s",s_tid, __func__, pathname);

	char          name[128],dup_name[128];
	char 	      *d_name,  *b_name;
	Inode*        dp;
	Inode*        ip;
	Inode*        gp;
//	mfs::client::FileInode* fip;
	int           ret;

  
      #ifdef CONFIG_CACHE
/*	strcpy(dup_name, pathname);
	Lookup(dup_name, (void **)&ip, (void **)&dp);

	strcpy(dup_name, pathname);
	Lookup(dirname(dup_name), (void **)&dp, (void **)&gp);
	
	dp->Get();
	dp->Lock(session, gp, lock_protocol::Mode::XL);
	ip->Get();

	strcpy(dup_name, pathname);
	b_name = basename(dup_name);
	strcpy(name, b_name); 	  
*/	strcpy(dup_name, pathname);
        Erase(dup_name, ip, dp);

      #endif

	DBG_LOG(DBG_INFO, DBG_MODULE(client_name), "Unlink: %s\n", pathname);

	// we do spider locking; when Nameiparent returns successfully, dp is 
	// locked for writing. we release the lock on dp after we get the lock
	// on its child
	if ((ret = global_namespace->Nameiparent(session, pathname, lock_protocol::Mode::XL, 
	                                         name, &dp)) < 0) 
	{
		return ret;
	}

	// Cannot unlink "." or "..".
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		dp->Put();
		dp->Unlock(session);
		return -E_INVAL;
	}

	if ((ret = dp->Lookup(session, name, 0, &ip)) != E_SUCCESS) {
		dp->Put();
		dp->Unlock(session);
		return ret;
	}

	
	// do we need recursive lock if ip is a file? 
	ip->Lock(session, dp, lock_protocol::Mode::XR); 
	assert(ip->nlink() > 0);

	if (ip->type() == kDirInode) {
		bool isempty;
		assert(ip->ioctl(session, 1, &isempty) == E_SUCCESS);
		if (!isempty) {
			ip->Put();
			ip->Unlock(session);
			dp->Put();
			dp->Unlock(session);
			return -E_NOTEMPTY;
		}
	}

	// BUG: We should defer deleting a file opened by us because as soon as
	// we publish our operation to the server, the server we free and recycle 
	// storage associated with the file. 

	session->journal()->TransactionBegin();
	session->journal() << Publisher::Message::LogicalOperation::Unlink(dp->ino(), name);
	
	assert(dp->Unlink(session, name) == E_SUCCESS);
	//FIXME: inode's link/unlink should take care of the nlink
	if (ip->type() == kDirInode) {
		assert(dp->set_nlink(dp->nlink() - 1) == 0); // for child's backlink ..
	}
	assert(ip->set_nlink(ip->nlink() - 1) == 0); // for parent's forward link
	
	session->journal()->TransactionCommit();

	dp->Put();
	dp->Unlock(session);
	ip->Put();
	if (ip->ref_count() == 0 && ip->nlink() == 0) {
		if ((ret = global_fsomgr->DestroyInode(session, ip)) < 0) {
			DBG_LOG(DBG_CRITICAL, DBG_MODULE(client_name), "Cannot destroy inode: %llu", ip->ino());
		}
	} else {
		ip->Unlock(session);
	}

	return E_SUCCESS;

}

} // namespace client
