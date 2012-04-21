/**
 * \file omgr.h
 *
 * \brief Generic persistent object (container) manager
 * 
 */

#ifndef __STAMNOS_OSD_CLIENT_OBJECT_MANAGER_H
#define __STAMNOS_OSD_CLIENT_OBJECT_MANAGER_H

#include <google/dense_hash_map>
#include "osd/main/common/obj.h"
#include "osd/main/client/proxy.h"
#include "osd/main/client/omap.h"
#include "osd/main/client/hlckmgr.h"
#include "osd/main/client/osd-opaque.h"
#include "osd/main/client/session.h"
#include "osd/containers/containers.h"


namespace osd {
namespace client {

class ObjectManager; // forward declaration

/**
 * \brief Abstract-base class of type specific persistent object manager
 */
class ObjectManagerOfType {
friend class ObjectManager;
public:
	virtual ObjectProxy* Load(OsdSession* session, ObjectId oid) = 0;
	virtual void Close(OsdSession* session, ObjectId oid, bool update) = 0;
	virtual void CloseAll(OsdSession* session, bool update) = 0;

protected:	
	ObjectMap oid2obj_map_;
};


class ObjectManager: public osd::cc::client::HLockCallback, 
                     public osd::cc::client::LockCallback 
{
	typedef google::dense_hash_map<ObjectType, ObjectManagerOfType*> ObjectType2Manager; 
public:
	ObjectManager(osd::client::StorageSystem* stsystem);
	~ObjectManager();
	int RegisterType(ObjectType type_id, ObjectManagerOfType* mgr);
	int FindOrGetObject(OsdSession* session, ObjectId oid, osd::common::ObjectProxyReference** obj_ref); 
	int FindObject(OsdSession* session, ObjectId oid, osd::common::ObjectProxyReference** obj_ref); 
	int GetObject(OsdSession* session, ObjectId oid, osd::common::ObjectProxyReference** obj_ref);
	int PutObject(OsdSession* session, osd::common::ObjectProxyReference& obj_ref);
	int CloseObject(OsdSession* session, ObjectId oid, bool update);
	int id() { return id_; }
	void CloseAllObjects(OsdSession* session, bool update);

	// call-back methods
	void PreDowngrade();
	void OnRelease(osd::cc::client::HLock* hlock);
	void OnConvert(osd::cc::client::HLock* hlock);
	void OnRelease(osd::cc::client::Lock* lock);
	void OnConvert(osd::cc::client::Lock* lock);

private:
	int GetObjectInternal(OsdSession* session, ObjectId oid, osd::common::ObjectProxyReference** obj_ref, bool use_exist_obj_ref);
	int RegisterBaseTypes();
	
	int                          id_;
	pthread_mutex_t              mutex_;
	ObjectManagerOfType*         objtype2mgr_tbl_[osd::containers::T_CONTAINER_TYPE_COUNT];
	osd::client::StorageSystem*  stsystem_;
	OsdSession*                  cb_session_; /**< the session used when calling call-back methods */
};


} // namespace client
} // namespace osd

#endif // __STAMNOS_OSD_CLIENT_OBJECT_MANAGER_H
