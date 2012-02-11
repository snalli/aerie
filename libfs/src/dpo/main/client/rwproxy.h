/**
 * \file rwproxy.h
 *
 * \brief Read-write proxy to a read-only persistent object
 * 
 */

#ifndef __STAMNOS_DPO_CLIENT_RW_PROXY_H
#define __STAMNOS_DPO_CLIENT_RW_PROXY_H

#include "dpo/main/client/proxy.h"
#include <assert.h>
#include "dpo/main/client/omgr.h"
#include "client/session.h"

namespace dpo {
namespace client {
namespace rw {


template<class Subject, class VersionManager> class ObjectProxy;

/**
 * \brief Implementation of type specific persistent object manager
 */
template<class Subject, class VersionManager>
class ObjectManager: public dpo::client::ObjectManagerOfType {
public:
	dpo::client::ObjectProxy* Load(::client::Session* session, ObjectId oid) {
		return new ObjectProxy<Subject, VersionManager>(session, oid);
	}
	
	void Publish(::client::Session* session, ObjectId oid) {
		ObjectProxy<Subject, VersionManager>* obj_proxy;
		dpo::client::ObjectProxy*             obj2_proxy;
		assert(oid2obj_map_.Lookup(oid, &obj2_proxy) == E_SUCCESS);
		obj_proxy = static_cast<ObjectProxy<Subject, VersionManager>* >(obj2_proxy);
		assert(obj_proxy->vClose(session) == E_SUCCESS);
	}
};


template<class Subject, class VersionManager>
class ObjectProxyReference: public dpo::common::ObjectProxyReference {
public:
	ObjectProxyReference(void* owner = NULL)
		: dpo::common::ObjectProxyReference(owner)
	{ }

	ObjectProxy<Subject, VersionManager>* proxy() { 
		return static_cast<ObjectProxy<Subject, VersionManager>*>(proxy_);
	}
};


template<class Subject, class VersionManager>
class ObjectProxy: public dpo::vm::client::ObjectProxy< ObjectProxy<Subject, VersionManager>, Subject, VersionManager> {
public:
	ObjectProxy(::client::Session* session, ObjectId oid)
		: dpo::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>(session, oid),
	      session_(session)
	{ }

	int Lock(::client::Session* session, lock_protocol::Mode mode) {
		int ret;
		
		// FIXME: check if object is private or public. if public then lock. 

		if ((ret = dpo::cc::client::ObjectProxy::Lock(session, mode)) != lock_protocol::OK) {
			return ret;
		}
		assert((dpo::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>::vOpen() == 0));
		return lock_protocol::OK;
	}

	int Lock(::client::Session* session, dpo::cc::client::ObjectProxy* parent, lock_protocol::Mode mode) {
		int ret;
		
		// FIXME: check if object is private or public. if public then lock. 
		
		if ((ret = dpo::cc::client::ObjectProxy::Lock(session, parent, mode)) != lock_protocol::OK) {
			return ret;
		}
		assert((dpo::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>::vOpen() == 0));
		return lock_protocol::OK;

	}

	int Unlock(::client::Session* session) {
		// FIXME: check if object is private or public. if public then unlock. 
		return dpo::cc::client::ObjectProxy::Unlock(session);
	}

private:
	::client::Session* session_;
};


} // namespace rw
} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_RWPROXY_H
