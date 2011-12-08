/**
 * \file rw.h
 *
 * \brief Read-write proxy to a read-only persistent object
 * 
 */

#ifndef __STAMNOS_DPO_CLIENT_RW_H
#define __STAMNOS_DPO_CLIENT_RW_H

namespace dpo {

namespace rw {

template<class Derived>
class ObjectProxy: public dcc::client::Object {
public:
	int Lock() {
		dcc::Object::Lock();
		CopyOnWrite();
	}

	int CopyOnWrite() {
		if (state_ == INVALID) {
			Derived::CopyOnWrite();
			state_ = VALID;
		}
		return 0;
	}

	// this function has to be virtual or functor that is registered 
	// with the object manager
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

private:
	state_; // invalid, opened (valid)

};

template<class Proxy, class Subject>
class ObjectProxy {
public:
	Subject* subject() { return subject_; }
	void setSubject(Subject* subject) { subject_ = subject; }
private:
	Subject* subject_;
};

} // namespace rw

} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_RW_H
