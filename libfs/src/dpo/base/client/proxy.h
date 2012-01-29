#ifndef __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
#define __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H

#include "common/errno.h"
#include "dpo/base/common/proxy.h"
#include "dpo/base/client/hlckmgr.h"
#include "dpo/base/client/stm.h"
#include "client/session.h"

namespace dpo {

namespace client {

typedef dpo::common::ObjectProxy ObjectProxy;
typedef dpo::common::ObjectId    ObjectId; 
typedef dpo::common::ObjectType  ObjectType;

} // namespace client


// CONCURRENCY CONTROL

namespace cc {
namespace client {

class ObjectProxy: public dpo::common::ObjectProxy {
public:
	ObjectProxy(::client::Session* session, dpo::common::ObjectId oid) 
		: dpo::common::ObjectProxy(oid)
	{ 
		hlock_ = session->hlckmgr_->FindOrCreateLock(LockId(1, oid.num()));
		hlock_->set_payload(reinterpret_cast<void*>(oid.u64()));
	}

	int Lock(::client::Session* session, lock_protocol::Mode mode) {
		return session->hlckmgr_->Acquire(hlock_, mode, 0);
	}

	int Lock(::client::Session* session, dpo::cc::client::ObjectProxy* parent, lock_protocol::Mode mode) {
		assert(parent->hlock_);
		return session->hlckmgr_->Acquire(hlock_, parent->hlock_, mode, 0);
	}

	int Unlock(::client::Session* session) {
		return session->hlckmgr_->Release(hlock_);
	}

private:
	dpo::cc::client::HLock* hlock_; // hierarchical lock
	dpo::cc::client::Lock*  lock_;  // flat lock 
};


template<class Derived, class Subject>
class ObjectProxyTemplate: public ObjectProxy {
public:
	ObjectProxyTemplate(::client::Session* session, dpo::common::ObjectId oid)
		: ObjectProxy(session, oid)
	{ }

	Subject* object() { return static_cast<Subject*>(object_); }

	Derived* xOpenRO(dpo::stm::client::Transaction* tx);
	Derived* xOpenRO();

private:
	dpo::stm::client::Transaction* tx_;
};


template<class Derived, class Subject>
Derived* ObjectProxyTemplate<Derived, Subject>::xOpenRO(dpo::stm::client::Transaction* tx)
{
	Derived* derived = static_cast<Derived*>(this);
	Subject* subj = derived->object();
	tx->OpenRO(subj);
	return derived;
}


template<class Derived, class Subject>
Derived* ObjectProxyTemplate<Derived, Subject>::xOpenRO()
{
	dpo::stm::client::Transaction* tx = dpo::stm::client::Self();
	return xOpenRO(tx);
}


} // namespace client
} // namespace cc


// VERSION MANAGEMENT

namespace vm {
namespace client {


// This class is inherited by the VersionManager class specific to each object. 
// It must provide the same interface as the underlying object
template<class Subject>
class VersionManager {
public:
	VersionManager(Subject* object = NULL)
		: object_(object),
		  nlink_(0)
	{ }

	void set_object(Subject* object) {
		object_ = object;
	}
	
	Subject* object() {
		return object_;
	}

	int vOpen() {
		nlink_ = object_->nlink();
		return 0;
	}
	
	int vUpdate(::client::Session* session) {
		//FIXME: updates must go to the journal and done by the server
		object_->set_nlink(nlink_);
		//FIXME: version counter must be updated by the server
		object_->ccSetVersion(object_->ccVersion() + 1);
		return 0;
	}

	int nlink() {
		return nlink_;
	}

	int set_nlink(int nlink) {
		nlink_ = nlink;
		return 0;
	}

protected:
	Subject* object_;
	int      nlink_;
};


// FIXME: Currently we rely on composition to parameterize 
// ObjectProxy on the VersionManager. 
// Can we make VersionManager a mixin layer instead? This would 
// save us from the extra object_ kept in the VersionManager class
// and avoid the call to the ugly interface() to access the shadow object

template<class Derived, class Subject, class VersionManager>
class ObjectProxy: public dpo::cc::client::ObjectProxyTemplate<Derived, Subject>
{
public:
	ObjectProxy(::client::Session* session, dpo::common::ObjectId oid)
		: dpo::cc::client::ObjectProxyTemplate<Derived, Subject>(session, oid),
		  valid_(false)
	{ 
		// initializing in the initialization list is risky as we need to be 
		// sure that dpo::cc::client::ObjectProxyTemplate<Derived, Subject>
		// has been initialized first.
		vm_.set_object(dpo::cc::client::ObjectProxyTemplate<Derived, Subject>::object());
	}

	// provides access to the buffered state
	VersionManager* interface() {
		return &vm_;
	}

	// for accessing the object when using the RO-STM mechanism
	// provides direct access to the underlying object. it bypasses any buffering
	// done by the copy-on-right mechanism.
	Subject* xinterface() {
		return dpo::cc::client::ObjectProxyTemplate<Derived, Subject>::object();
	}

	int vOpen() {
		int ret;
		if (!valid_) {
			if ((ret = vm_.vOpen()) < 0) {
				return ret;
			}
			valid_ = true;
		}
		return 0;
	}
	
	int vUpdate(::client::Session* session) {
		return (valid_ ? vm_.vUpdate(session): E_INVAL);
	}

	int vClose(::client::Session* session) {
		int ret = E_SUCCESS;
		if (valid_) {
			ret = vm_.vUpdate(session);
		}
		valid_ = false;
		return ret;
	}

protected:
	bool           valid_; // invalid, opened (valid)
	VersionManager vm_;
};


} // namespace client
} // namespace vm

} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
