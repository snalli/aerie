/**
 * \file rwproxy.h
 *
 * \brief Read-write proxy to a read-only persistent object
 * 
 */

#ifndef __STAMNOS_DPO_CLIENT_RW_PROXY_H
#define __STAMNOS_DPO_CLIENT_RW_PROXY_H

#include <assert.h>
#include "dpo/client/proxy.h"
#include "dpo/client/omgr.h"

namespace dpo {

namespace client {

namespace rw {


template<class Subject, class VersionManager> class ObjectProxy;


template<class Subject, class VersionManager>
class ObjectManager: public dpo::client::ObjectManagerOfType {
public:
	dpo::client::ObjectProxy* Create(ObjectId oid) {
		return new ObjectProxy<Subject, VersionManager>(oid);
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
	ObjectProxy(ObjectId oid)
		: dpo::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>(oid)
	{ }

	int Lock(lock_protocol::Mode mode) {
		int ret;
		
		if ((ret = dpo::cc::client::ObjectProxy::Lock(mode)) != lock_protocol::OK) {
			return ret;
		}
		assert((dpo::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager>::vOpen() == 0));
		return lock_protocol::OK;
	}

	int Lock(dpo::cc::client::ObjectProxy* parent, lock_protocol::Mode mode) {
		// TODO
	}

	//Create(); // lazy shadow
	//Destroy();

	// this function has to be virtual or functor that is registered 
	// with the object manager
/*
	int Update(int flags) {
		if (state_ == VALID) {
			if (ret = Derived::Update() < 0) { 
				return ret;
			}
			if (flags & LOCK_REVOKE) {
				// lock is revoked so object proxy will get out of sync
				// sooner or later; mark the object as invalid
				state_ = INVALID; 
			}
			return 0;
		}
		return -1;
	}
*/
};


} // namespace rw
} // namespace client
} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_RWPROXY_H
