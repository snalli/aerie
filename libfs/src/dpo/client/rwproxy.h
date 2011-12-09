/**
 * \file rw.h
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

template<class Derived, class Subject, class VersionManager>
//class ObjectProxy: public dpo::vm::client::ObjectProxy<ObjectProxy<Subject, VersionManager>, Subject, VersionManager> {
class ObjectProxy: public dpo::vm::client::ObjectProxy<Derived, Subject, VersionManager> {
public:
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
