/**
 * \file omgr.cc 
 *
 * \brief Object proxy manager
 */

#include "osd/main/client/omgr.h"
#include "common/errno.h"
#include "common/prof.h"
#include "bcs/bcs.h"
#include "osd/main/client/stsystem.h"
#include "osd/main/client/lckmgr.h"
#include "osd/main/client/hlckmgr.h"
#include "osd/main/client/rwproxy.h"
#include "osd/main/client/session.h"
#include "osd/containers/super/container.h"
#include "osd/containers/name/container.h"
#include "osd/containers/byte/container.h"
#include "osd/containers/needle/container.h"


//TODO: Fine-grain locking in GetObject/PutObject.

//#undef  PROFILER_SAMPLE
//#define PROFILER_SAMPLE __PROFILER_SAMPLE

namespace osd {
namespace client {


/**
 * The storage system must have properly initialized lock managers
 */
ObjectManager::ObjectManager(osd::client::StorageSystem* stsystem)
	: stsystem_(stsystem)
{
	pthread_rwlock_init(&rwlock_, NULL);
	for (int i=0; i < osd::containers::T_CONTAINER_TYPE_COUNT; i++) {
		objtype2mgr_tbl_[i] = NULL;
	}
	stsystem_->hlckmgr()->RegisterLockCallback(this);
	stsystem_->lckmgr()->RegisterLockCallback(::osd::cc::client::Lock::TypeId, this);
	id_ = stsystem_->ipc()->id();
	cb_session_ = new OsdSession(stsystem_);
	assert(RegisterBaseTypes() == E_SUCCESS);
}


ObjectManager::~ObjectManager()
{
//	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
//	        "[%d] Shutting down Object Manager\n", id());

	stsystem_->hlckmgr()->UnregisterLockCallback();
	stsystem_->lckmgr()->UnregisterLockCallback(::osd::cc::client::Lock::TypeId);

}


int
ObjectManager::RegisterBaseTypes()
{
	int ret;

	// SuperContainer
	{
	osd::client::rw::ObjectManager<osd::containers::client::SuperContainer::Object, osd::containers::client::SuperContainer::VersionManager>* mgr = new osd::client::rw::ObjectManager<osd::containers::client::SuperContainer::Object, osd::containers::client::SuperContainer::VersionManager>;
    if ((ret = RegisterType(osd::containers::T_SUPER_CONTAINER, mgr)) < 0) {
		return ret;
	}
	}

	{
	// NameContainer
	osd::client::rw::ObjectManager<osd::containers::client::NameContainer::Object, osd::containers::client::NameContainer::VersionManager>* mgr = new osd::client::rw::ObjectManager<osd::containers::client::NameContainer::Object, osd::containers::client::NameContainer::VersionManager>;
    if ((ret = RegisterType(osd::containers::T_NAME_CONTAINER, mgr)) < 0) {
		return ret;
	}
	}

	{
	// ByteContainer
	osd::client::rw::ObjectManager<osd::containers::client::ByteContainer::Object, osd::containers::client::ByteContainer::VersionManager>* mgr = new osd::client::rw::ObjectManager<osd::containers::client::ByteContainer::Object, osd::containers::client::ByteContainer::VersionManager>;
    if ((ret = RegisterType(osd::containers::T_BYTE_CONTAINER, mgr)) < 0) {
		return ret;
	}
	}

	{
	// NeedleContainer
	osd::client::rw::ObjectManager<osd::containers::client::NeedleContainer::Object, osd::containers::client::NeedleContainer::VersionManager>* mgr = new osd::client::rw::ObjectManager<osd::containers::client::NeedleContainer::Object, osd::containers::client::NeedleContainer::VersionManager>;
    if ((ret = RegisterType(osd::containers::T_NEEDLE_CONTAINER, mgr)) < 0) {
		return ret;
	}
	}

	return E_SUCCESS;
}


int
ObjectManager::RegisterType(ObjectType type_id, ObjectManagerOfType* mgr)
{
	int                          ret = E_SUCCESS;
	ObjectType2Manager::iterator itr; 

	if (type_id >= osd::containers::T_CONTAINER_TYPE_COUNT) {
		return -E_INVAL;
	}

	pthread_rwlock_wrlock(&rwlock_);
	if (objtype2mgr_tbl_[type_id]) {
		ret = -E_EXIST;
		goto done;
	}
	objtype2mgr_tbl_[type_id] = mgr;

done:
	pthread_rwlock_unlock(&rwlock_);
	return ret;
}


/**
 * \brief Initializes a reference to the object identified by oid
 *
 * If use_exist_obj_ref is set then it sets obj_ref to point to an existing
 * reference that points to the object. This is useful for embedded references
 * Otherwise it initializes the obj_ref to point to the object. 
 */
int
ObjectManager::GetObjectInternal(OsdSession* session,
                                 ObjectId oid, 
                                 osd::common::ObjectProxyReference** objproxy_refp, 
                                 bool use_exist_obj_ref)
{
	int                                ret = E_SUCCESS;
	ObjectManagerOfType*               mgr;
	ObjectProxy*                       objproxy;
	osd::common::ObjectProxyReference* objproxy_ref;
	bool                               rwlock = false;

	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
	        "[%d] Object: oid=%lx, type=%d\n", id(), oid.u64(), oid.type());

lock:
	if (rwlock) {
		pthread_rwlock_wrlock(&rwlock_);
	} else {
		pthread_rwlock_rdlock(&rwlock_);
	}
	ObjectType type = oid.type();
	if (type >= osd::containers::T_CONTAINER_TYPE_COUNT || !(mgr = objtype2mgr_tbl_[type])) { 
		ret = -E_INVAL; // unknown type id 
		goto done;
	}
	if ((ret = mgr->oid2obj_map_.Lookup(oid, &objproxy)) != E_SUCCESS) {
		if (!rwlock) {
			rwlock=true;
			pthread_rwlock_unlock(&rwlock_);
			goto lock;
		}
		// create the object proxy
		if ((objproxy = mgr->Load(cb_session_, oid)) == NULL) {
			ret = -E_NOMEM;
			goto done;
		}
		assert(mgr->oid2obj_map_.Insert(objproxy) == E_SUCCESS);
		// no need to grab the lock on obj after creation as it's not reachable 
		// before we release the lock manager's mutex lock
		objproxy_ref = new osd::common::ObjectProxyReference();
		objproxy_ref->Set(objproxy, false);
	} else {
		// object proxy exists
		if (use_exist_obj_ref) {
			if ((objproxy_ref = objproxy->HeadReference()) == NULL) {
				if (!rwlock) {
					rwlock=true;
					pthread_rwlock_unlock(&rwlock_);
					goto lock;
				}
				objproxy_ref = new osd::common::ObjectProxyReference();
				objproxy_ref->Set(objproxy, false);
			}
		} else {
			if (!rwlock) {
				rwlock=true;
				pthread_rwlock_unlock(&rwlock_);
				goto lock;
			}
			objproxy_ref = new osd::common::ObjectProxyReference();
			objproxy_ref->Set(objproxy, true);
		}
	}
	ret = E_SUCCESS;
	*objproxy_refp = objproxy_ref;
done:
	pthread_rwlock_unlock(&rwlock_);
	return ret;
}


/**
 * \brief It returns a reference to the object proxy if the object identified 
 * by oid exists.
 * If a reference already exists it returns the existing reference without
 * incrementing the refcount.
 * If a reference does not exist it creates and returns a new reference 
 * (with refcount=1)
 *
 */
int
ObjectManager::FindObject(OsdSession* session, 
                          ObjectId oid, 
                          osd::common::ObjectProxyReference** obj_ref) 
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
	        "[%d] Object: oid=%lx, type=%d\n", id(), oid.u64(), oid.type());
	
	return GetObjectInternal(session, oid, obj_ref, true);
}


/**
 * \brief It returns a reference to the object proxy if the object identified by
 * oid exists. It increments the reference count.
 *
 */
int
ObjectManager::GetObject(OsdSession* session, 
                         ObjectId oid, 
                         osd::common::ObjectProxyReference** obj_ref) 
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), 
	        "[%d] Object: oid=%lx, type=%d\n", id(), oid.u64(), oid.type());
	
	return GetObjectInternal(session, oid, obj_ref, false);
}


int
ObjectManager::PutObject(OsdSession* session, 
                         osd::common::ObjectProxyReference& obj_ref)
{
	int                  ret = E_SUCCESS;

	pthread_rwlock_wrlock(&rwlock_);
	obj_ref.Reset(true);
	pthread_rwlock_unlock(&rwlock_);
	return ret;
}


int
ObjectManager::CloseObject(OsdSession* session, ObjectId oid, bool update)
{
	int                  ret = E_SUCCESS;
	ObjectManagerOfType* mgr;

	pthread_rwlock_wrlock(&rwlock_);
	ObjectType type = oid.type();
	ObjectType2Manager::iterator itr;
	if (type >= osd::containers::T_CONTAINER_TYPE_COUNT || !(mgr = objtype2mgr_tbl_[type])) {
		goto done;
	}
	mgr->Close(session, oid, update);

done:
	pthread_rwlock_unlock(&rwlock_);
	return ret;
}


/**
 * \brief Call-back made by the lock manager when a hierarchical lock is released 
 */
void 
ObjectManager::OnRelease(osd::cc::client::HLock* hlock)
{
	ObjectId oid(reinterpret_cast<uint64_t>(hlock->payload()));

	if (hlock->sticky_) {
		osd::common::Object* obj = osd::common::Object::Load(oid);
		obj->incr_dlink(1);
	}

	assert(CloseObject(cb_session_, oid, false) == E_SUCCESS);
}


/**
 * \brief Call-back made by the lock manager when a hierarchical lock is down-graded 
 */
void 
ObjectManager::OnConvert(osd::cc::client::HLock* hlock)
{
	ObjectId oid(reinterpret_cast<uint64_t>(hlock->payload()));

	assert(CloseObject(cb_session_, oid, false) == E_SUCCESS);
}


/**
 * \brief Call-back made by the lock manager when a flat lock is released 
 */
void 
ObjectManager::OnRelease(osd::cc::client::Lock* lock)
{
	ObjectId oid(reinterpret_cast<uint64_t>(lock->payload()));

	assert(CloseObject(cb_session_, oid, false) == E_SUCCESS);
}


/**
 * \brief Call-back made by the lock manager when a flat lock is down-graded 
 */
void 
ObjectManager::OnConvert(osd::cc::client::Lock* lock)
{
	ObjectId oid(reinterpret_cast<uint64_t>(lock->payload()));

	assert(CloseObject(cb_session_, oid, false) == E_SUCCESS);
}


void
ObjectManager::CloseAllObjects(OsdSession* session, bool update)
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_omgr), "[%d] Close all objects\n", id());
	
	ObjectManagerOfType*         mgr;
	ObjectType2Manager::iterator itr;
	
	pthread_rwlock_wrlock(&rwlock_);
	
	// flush the log of updates to the server
	if (update) {
		/*
		if ((ret = stsystem_->ipc()->call(::osd::Publisher::Protocol::kPublish, 
		                                  stsystem_->ipc()->id(), unused)) < 0) {

			DBG_LOG(DBG_WARNING, DBG_MODULE(client_omgr), "[%d] Flush failed\n", id());
		}
		*/
		stsystem_->shbuf()->SignalReader();
	}

	// now close all the objects 
	for (int i=0; i < osd::containers::T_CONTAINER_TYPE_COUNT; i++) {
		if (mgr = objtype2mgr_tbl_[i]) {
			mgr->CloseAll(session, false);
		}
	}

	pthread_rwlock_unlock(&rwlock_);
}


void
ObjectManager::PreDowngrade()
{
	CloseAllObjects(cb_session_, true);
}


} // namespace client
} // namespace osd
