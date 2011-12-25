/**
 * \file rwproxy.h
 *
 * \brief Read-write proxy to a read-only persistent object
 * 
 */

#ifndef __STAMNOS_DPO_CLIENT_RW_PROXY_H
#define __STAMNOS_DPO_CLIENT_RW_PROXY_H

#include <assert.h>
#include "dpo/base/client/proxy.h"
#include "dpo/base/client/omgr.h"
#include "client/session.h"

namespace dpo {
namespace client {
namespace rw {


template<class Subject, class VersionManager> class ObjectProxy;


template<class Subject, class VersionManager>
class ObjectManager: public dpo::client::ObjectManagerOfType {
public:
	dpo::client::ObjectProxy* Create(::client::Session* session, ObjectId oid) {
		return new ObjectProxy<Subject, VersionManager>(session, oid);
	}

	void OnRelease(::client::Session* session, ObjectId oid) {
		ObjectProxy<Subject, VersionManager>* obj;
		dpo::client::ObjectProxy*             obj2;
		assert(oid2obj_map_.Lookup(oid, &obj2) == E_SUCCESS);
		obj = static_cast<ObjectProxy<Subject, VersionManager>* >(obj2);
		assert(obj->vClose(session) == E_SUCCESS);
	}
};


template<class Subject, class VersionManager>
class ObjectProxyReference: public dpo::common::ObjectProxyReference {
public:
	ObjectProxy<Subject, VersionManager>* obj() { 
		return static_cast<ObjectProxy<Subject, VersionManager>*>(obj_);
	}
};


template<class Subject, class VersionManager>
class ObjectProxy: public dpo::vm::client::ObjectProxy< ObjectProxy<Subject, VersionManager>, Subject, VersionManager> {
public:
	ObjectProxy(::client::Session* session, ObjectId oid)
		: dpo::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>(session, oid),
	      session_(session)
	{ }

	int Lock(lock_protocol::Mode mode) {
		int ret;
		
		if ((ret = dpo::cc::client::ObjectProxy::Lock(session_, mode)) != lock_protocol::OK) {
			return ret;
		}
		assert((dpo::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>::vOpen() == 0));
		return lock_protocol::OK;
	}

	int Lock(dpo::cc::client::ObjectProxy* parent, lock_protocol::Mode mode) {
		int ret;
		
		if ((ret = dpo::cc::client::ObjectProxy::Lock(session_, parent, mode)) != lock_protocol::OK) {
			return ret;
		}
		assert((dpo::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>::vOpen() == 0));
		return lock_protocol::OK;

	}

	int Unlock() {
		return dpo::cc::client::ObjectProxy::Unlock(session_);
	}

private:
	::client::Session* session_;
};


} // namespace rw
} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_RWPROXY_H
