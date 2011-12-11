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

#if 0
template<class Derived, class Subject, class VersionManager> class ObjectProxyTemplate;


//class ObjectManager: public ObjectManagerOfType {
//public:
//};


template<class Derived, class Subject, class VersionManager>
class ObjectProxyReferenceTemplate {
public:
 

private:
	ObjectProxyTemplate<Derived, Subject, VersionManager>*          obj_;
	ObjectProxyReferenceTemplate<Derived, Subject, VersionManager>* next_;
};


template<class Derived, class Subject, class VersionManager>
class ObjectProxyTemplate: public dpo::vm::client::ObjectProxy<Derived, Subject, VersionManager> {
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
	ObjectProxyReferenceTemplate<Derived, Subject, VersionManager>* next_;
};


template<class Subject, class VersionManager> 
class ObjectProxy: public ObjectProxyTemplate< ObjectProxy<Subject, VersionManager>, Subject, VersionManager> {

};

template<class Subject, class VersionManager>
class ObjectProxyReference {
public:
 

//private:
	ObjectProxy<Subject, VersionManager>*          obj_;
	ObjectProxyReference<Subject, VersionManager>* next_;
};

#endif


template<class Subject, class VersionManager> class ObjectProxy;


//class ObjectManager: public ObjectManagerOfType {
//public:
//};


template<class Subject, class VersionManager>
class ObjectProxyReference {
public:
 

//private:
	ObjectProxy<Subject, VersionManager>*          obj_;
	ObjectProxyReference<Subject, VersionManager>* next_;
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
	ObjectProxyReference<Subject, VersionManager>* next_;
};


} // namespace rw

} // namespace client

} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_RWPROXY_H
