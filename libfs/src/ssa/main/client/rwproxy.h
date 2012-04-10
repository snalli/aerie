/**
 * \file rwproxy.h
 *
 * \brief Read-write proxy to a read-only persistent object
 * 
 */

#ifndef __STAMNOS_SSA_CLIENT_RW_PROXY_H
#define __STAMNOS_SSA_CLIENT_RW_PROXY_H

#include "ssa/main/client/proxy.h"
#include <assert.h>
#include "ssa/main/client/omgr.h"
#include "ssa/main/client/session.h"


namespace ssa {
namespace client {
namespace rw {

typedef ::ssa::client::SsaSession SsaSession;

template<class Subject, class VersionManager> class ObjectProxy;

/**
 * \brief Implementation of type specific persistent object manager
 */
template<class Subject, class VersionManager>
class ObjectManager: public ssa::client::ObjectManagerOfType {
public:
	ssa::client::ObjectProxy* Load(SsaSession* session, ObjectId oid) {
		ssa::client::ObjectProxy* obj = new ObjectProxy<Subject, VersionManager>(session, oid);
		return obj;
	}
	
	void Close(SsaSession* session, ObjectId oid, bool update) {
		ObjectProxy<Subject, VersionManager>* obj_proxy;
		ssa::client::ObjectProxy*             obj2_proxy;
		assert(oid2obj_map_.Lookup(oid, &obj2_proxy) == E_SUCCESS);
		obj_proxy = static_cast<ObjectProxy<Subject, VersionManager>* >(obj2_proxy);
		assert(obj_proxy->vClose(session, update) == E_SUCCESS);
	}
	
	void CloseAll(SsaSession* session, bool update) {
		ObjectProxy<Subject, VersionManager>* obj_proxy;
		ssa::client::ObjectProxy*             obj2_proxy;
		ObjectMap::iterator                   itr;

		for (itr = oid2obj_map_.begin(); itr != oid2obj_map_.end(); itr++) {
			obj2_proxy = itr->second;
			DBG_LOG(DBG_DEBUG, DBG_MODULE(client_omgr), "Close object: %lx\n", 
			        obj2_proxy->object()->oid().u64());
			obj_proxy = static_cast<ObjectProxy<Subject, VersionManager>* >(obj2_proxy);
			assert(obj_proxy->vClose(session, update) == E_SUCCESS);
		}
	}
};


template<class Subject, class VersionManager>
class ObjectProxyReference: public ssa::common::ObjectProxyReference {
public:
	ObjectProxyReference(void* owner = NULL)
		: ssa::common::ObjectProxyReference(owner)
	{ }

	ObjectProxy<Subject, VersionManager>* proxy() { 
		return static_cast<ObjectProxy<Subject, VersionManager>*>(proxy_);
	}
};



/**
 *
 *
 *
 */
template<class Subject, class VersionManager>
class ObjectProxy: public ssa::vm::client::ObjectProxy< ObjectProxy<Subject, VersionManager>, Subject, VersionManager> {
public:
	ObjectProxy(SsaSession* session, ObjectId oid)
		: ssa::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>(session, oid),
	      session_(session)
	{ }

	int Lock(SsaSession* session, lock_protocol::Mode mode) {
		int ret;
		
		// TODO: check if object is private or public. if public then lock. 

		if ((ret = ssa::cc::client::ObjectProxy::Lock(session, mode)) != lock_protocol::OK) {
			return ret;
		}
		assert((ssa::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>::vOpen() == 0));
		return lock_protocol::OK;
	}

	int Lock(SsaSession* session, ssa::cc::client::ObjectProxy* parent, lock_protocol::Mode mode) {
		int ret;
		
		// TODO: check if object is private or public. if public then lock. 
		
		if ((ret = ssa::cc::client::ObjectProxy::Lock(session, parent, mode)) != lock_protocol::OK) {
			return ret;
		}
		assert((ssa::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>::vOpen() == 0));
		return lock_protocol::OK;

	}

	int Unlock(SsaSession* session) {
		// TODO: check if object is private or public. if public then unlock. 
		return ssa::cc::client::ObjectProxy::Unlock(session);
	}

private:
	SsaSession* session_;
};


} // namespace rw
} // namespace client
} // namespace ssa

#endif // __STAMNOS_SSA_CLIENT_RWPROXY_H
