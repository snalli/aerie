#define  __CACHE_GUARD__

#include "pxfs/mfs/client/dir_inode.h"
#include "pxfs/mfs/client/file_inode.h"
#include "pxfs/mfs/client/inode_factory.h"

#include "bcs/bcs.h"
#include "pxfs/client/backend.h"
#include "osd/containers/containers.h"
#include "common/prof.h"
#include <stdio.h>

//#define PROFILER_SAMPLE __PROFILER_SAMPLE

namespace mfs {
namespace client {

pthread_rwlock_t InodeFactory::rwlock_ = PTHREAD_RWLOCK_INITIALIZER;

int
InodeFactory::LoadDirInode(::client::Session* session, 
                           ::osd::common::ObjectId oid, 
                           ::client::Inode** ipp)
{
	int                                ret = E_SUCCESS;
	osd::common::ObjectProxyReference* ref;
	DirInode*                          dip;
	bool                               wrlock = false;

lock:
	while (session->omgr_->FindObject(session, oid, &ref) != E_SUCCESS);


                ref->lock();
                if (ref->owner()) {
                        dip = reinterpret_cast<DirInode*>(ref->owner());
                } else {
                                dip = new DirInode(ref);
                                ref->set_owner(dip);
                }
	        *ipp = dip;
                ref->unlock();
/*
 * 	Uncomment this !
	// atomically get a reference to the persistent object and 
	// create the in-core Inode 
	if (wrlock) {
		pthread_rwlock_wrlock(&rwlock_);
	} else {
		pthread_rwlock_rdlock(&rwlock_);
	}
	if ((ret = session->omgr_->FindObject(session, oid, &ref)) == E_SUCCESS) {
		if (ref->owner()) {
			// the in-core inode already exists; just return this and 
			// we are done
			dip = reinterpret_cast<DirInode*>(ref->owner());
		} else {
			if (!wrlock) {
				pthread_rwlock_unlock(&rwlock_);
				pthread_rwlock_wrlock(&rwlock_);
			}
			if (ref->owner()) {
				dip = reinterpret_cast<DirInode*>(ref->owner());
			} else {
				//printf("\n LoadDirInode. Creating dirInode.");
				dip = new DirInode(ref);
				ref->set_owner(dip);
//				ref->set_cache(&(::client.cache));
			}
		}
	} else {
		if (wrlock == false) {
			pthread_rwlock_unlock(&rwlock_);
			wrlock = true;
			goto lock;
		}
		dip = new DirInode(ref);
	}
	pthread_rwlock_unlock(&rwlock_);
	*ipp = dip;
*/
	return E_SUCCESS;
}


int
InodeFactory::MakeDirInode(::client::Session* session, ::client::Inode** ipp)
{
	int                                               ret = E_SUCCESS;
	osd::containers::client::NameContainer::Object*   obj;

	if ((obj = osd::containers::client::NameContainer::Object::Make(session)) == NULL) {
		return -E_NOMEM;
	}
	if ((ret = LoadDirInode(session, obj->oid(), ipp)) < 0) {
		// FIXME: deallocate the allocated persistent object (container)
		return ret;
	}
	return ret;
}


int
InodeFactory::LoadFileInode(::client::Session* session, 
                            ::osd::common::ObjectId oid, 
                            ::client::Inode** ipp)
{
	int                                ret = E_SUCCESS;
	osd::common::ObjectProxyReference* ref;
	FileInode*                         fip;
	bool                               wrlock = false;

//printf("\nInside InodeFactory::LoadFileInode...");
	
lock:
	 while (session->omgr_->FindObject(session, oid, &ref) != E_SUCCESS);


                ref->lock();
                if (ref->owner()) {
                        fip = reinterpret_cast<FileInode*>(ref->owner());
                } else {
                                fip = new FileInode(ref);
                                ref->set_owner(fip);
                }
                *ipp = fip;
                ref->unlock();
/*
	// atomically get a reference to the persistent object and 
	// create the in-core Inode 
	if (wrlock) {
		pthread_rwlock_wrlock(&rwlock_);
	} else {
		pthread_rwlock_rdlock(&rwlock_);
	}

// insight : Looks like, we have two branches that create file inode and fill the address to it in *ipp.
// insight : The difference the two branches is something to do with locks.
// insight : KNOW ME : Currently, I theorise that there is a pool of inode objects and if the the object we want exist, simply reutrn it else create it.

	if ((ret = session->omgr_->FindObject(session, oid, &ref)) == E_SUCCESS) { 
	// insight : The question to ponder here is this. LoadFileInode does not know the origin of oid Object.
	// The oid object could have been created right before the call to LoadFileInode or is an existent one.
	// There are two sources of origin : Existent and Created. Can it happen that an oid that was created have an
	// objproxy_ref in the hash table but no inode ? If not we can avoid the call to FindObject.
//	printf("\n LoadFileInode. Branch 1.");

		if (ref->owner()) {
			// the in-core inode already exists; just return this and 
			// we are done
//			printf("\n LoadFileInode. Branch 1.1");
			fip = reinterpret_cast<FileInode*>(ref->owner()); // insight : This is not where we create the inode
		} else {
			if (!wrlock) {
				pthread_rwlock_unlock(&rwlock_);
				pthread_rwlock_wrlock(&rwlock_);
			}
//			printf("\n LoadFileInode. Branch 1.2");

			if (ref->owner()) {
//				printf("\n LoadFileInode. Branch 1.2.1");
				fip = reinterpret_cast<FileInode*>(ref->owner());// insight : This is not where we create the inode

			} else {
//				printf("\n LoadFileInode. Branch 1.2.2");
				fip = new FileInode(ref); // insight : This is where we create the inode

				ref->set_owner(fip);
//				ref->set_cache(&(cache));
			}
//			printf("\n* At the time of creation");
*			printf("\n * Inode : 0%x, %s", fip->printme());
			printf("\n * Refp  : 0%x, %s", fip->Rw_ref(->printme()));
			printf("\n * Proxy : 0%x, %s", fip->Rw_ref()->proxy()->printme());
			printf("\n * Object: 0%x, %s", fip->Rw_ref()->proxy()->object()->printme());
/

//                      fip->printme();
//                      fip->Rw_ref()->printme();
//                      fip->Rw_ref()->proxy()->printme();
//                      fip->Rw_ref()->proxy()->object()->printme();


		}
	} else {
		if (wrlock == false) {
			pthread_rwlock_unlock(&rwlock_);
			wrlock = true;
			goto lock;
		}
//	printf("\n LoadFileInode. Branch 2.");
		fip = new FileInode(ref); // insight : This is where we create the inode
		// insight : Why would you create the inode when you fail to find or create proxy ??????

	}
	pthread_rwlock_unlock(&rwlock_);
	*ipp = fip;
*/
	return ret;
}


int
InodeFactory::MakeFileInode(::client::Session* session, ::client::Inode** ipp)
{
	int                                               ret = E_SUCCESS;
	osd::containers::client::ByteContainer::Object*   obj;
// insight : This function has 2 arguments.
//
	DBG_LOG(DBG_INFO, DBG_MODULE(client_inode), "Create file inode\n");

// insight : This function has 1 argument.
//	printf("\nInside IndoeFactory::MakeFileInode...");
//	printf("\nCalling osd::containers::client::ByteContainer::Object::Make\n");
	if ((obj = osd::containers::client::ByteContainer::Object::Make(session)) == NULL) {
	// insight : Look for ByteContainer in osd/containers/byte/container-shadow*.h
	// insight : Look for Object in osd/containers/byte/common.h
	// insight : KNOW ME : Inode creation not fully understood. Need to look at session session->salloc()->AllocateContainer
		return -E_NOMEM;
	}
	DBG_LOG(DBG_INFO, DBG_MODULE(client_inode), "Create file inode: %p\n", (void*) obj->oid().u64());

	if ((ret = LoadFileInode(session, obj->oid(), ipp)) < 0) {
		// FIXME: deallocate the allocated object
		// insight : Systems needs common sense ! Something you make, something you destroy !
		return ret;
	}
	return ret;
}


int
InodeFactory::DestroyFileInode(::client::Session* session, ::client::Inode* ip)
{
	int                                ret = E_SUCCESS;
	osd::common::ObjectProxyReference* ref;
	FileInode*                         fip;

	pthread_rwlock_wrlock(&rwlock_);
	ref = ip->ref_;
	ref->set_owner(NULL);
	session->omgr_->CloseObject(session, ip->oid(), true);
	ip->Unlock(session);

	pthread_rwlock_unlock(&rwlock_);

	return ret;
}


int
InodeFactory::MakeInode(::client::Session* session, int type, ::client::Inode** ipp)
{
	int ret = E_SUCCESS;
// insight : This function has 3 arguments.
	switch (type) {
		case kDirInode:
			ret = MakeDirInode(session, ipp);
			break;
		case kFileInode:	
			ret = MakeFileInode(session, ipp);
			break;
		default:
			DBG_LOG (DBG_CRITICAL, DBG_MODULE(client_inode), "Unknown inode type\n");
	}
	return ret;
}


int 
InodeFactory::LoadInode(::client::Session* session, osd::common::ObjectId oid, 
                        ::client::Inode** ipp)
{
	int ret = E_SUCCESS;

	switch (oid.type()) {
		case osd::containers::T_NAME_CONTAINER: // directory
			ret = LoadDirInode(session, oid, ipp);
			break;
		case osd::containers::T_BYTE_CONTAINER: // insight : Fancy names. Clearly, T_*_CONTAINER is an integer because it is in a switch case.
			ret = LoadFileInode(session, oid, ipp);
			break;
		default: 
			DBG_LOG (DBG_CRITICAL, DBG_MODULE(client_inode), "Unknown container type\n");
	}
	
	return ret;
}


int 
InodeFactory::DestroyInode(::client::Session* session, ::client::Inode* ip)
{
	int ret = E_SUCCESS;

	switch (ip->oid().type()) {
		case osd::containers::T_NAME_CONTAINER: // directory
			// TODO
			// DO ME
			break;
		case osd::containers::T_BYTE_CONTAINER:
			ret = DestroyFileInode(session, ip);
			break;
		default: 
			DBG_LOG (DBG_CRITICAL, DBG_MODULE(client_inode), "Unknown container type\n");
	}
	
	return ret;
}


// WTF ?? Why this fancy programming ?
// Why another level of function call ?
int
InodeFactory::Make(::client::Session* session, int type, ::client::Inode** ipp)
{
	return MakeInode(session, type, ipp);
}


int 
InodeFactory::Load(::client::Session* session, osd::common::ObjectId oid, 
                   ::client::Inode** ipp)
{
	return LoadInode(session, oid, ipp);
}


int 
InodeFactory::Destroy(::client::Session* session, ::client::Inode* ip)
{
	return DestroyInode(session, ip);
}


} // namespace client 
} // namespace mfs
