/**
 * \file rwproxy.h
 *
 * \brief Read-write proxy to a read-only persistent object
 * 
 */

#ifndef __STAMNOS_DPO_CLIENT_RW_PROXY_H
#define __STAMNOS_DPO_CLIENT_RW_PROXY_H

#include "dpo/client/proxy.h"

namespace dpo {

namespace client {

namespace rw {


template<class Subject, class VersionManager> class ObjectProxy;


template<class Subject, class VersionManager>
class ObjectManager: public dpo::client::ObjectManagerOfType {
public:
	//FIXME: ObjectProxy<Subject, VersionManager>* Create() {
	dpo::client::ObjectProxy* Create(ObjectId oid) {
		return new ObjectProxy<Subject, VersionManager>;
	}


};



template<class Subject, class VersionManager>
class ObjectProxyReference: public dpo::common::ObjectProxyReference {
public:
 	//FIXME: these fields have been moved under dpo::common::ObjectProxyReference
	//       ../common/proxy.h         
	//FIXME: This class should only provide setter/getter methods to obj_ and next_
	//       for the specific Subject/versionmanager template instant

//private:
	//ObjectProxy<Subject, VersionManager>*          obj_;
	//ObjectProxyReference<Subject, VersionManager>* next_;
};


template<class Subject, class VersionManager>
class ObjectProxy: public dpo::vm::client::ObjectProxy< ObjectProxy<Subject, VersionManager>, Subject, VersionManager> {
public:
	//Create(); // lazy shadow
	//Destroy();
	//Lock();
	//Unlock();


	//int Lock(lock_protocol::Mode mode) {
		//dcc::Object::Lock();
		//CopyOnWrite();
	//}

/*
	int CopyOnWrite() {
		if (state_ == INVALID) {
			Derived::CopyOnWrite();
			state_ = VALID;
		}
		return 0;
	}
*/
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
