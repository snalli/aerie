#ifndef __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
#define __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H

#include "dpo/common/proxy.h"
#include "dpo/client/hlckmgr.h"
#include "dpo/client/stm.h"

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
	ObjectProxy(dpo::common::ObjectId oid) 
		: dpo::common::ObjectProxy(oid)
	{ }

	int Lock(lock_protocol::Mode mode) {

	}

	int Lock(dpo::cc::client::ObjectProxy* parent, lock_protocol::Mode mode) {

	}

	int Unlock() {

	}

private:
	//dpo::cc::client::HLock hlock;
};


template<class Derived, class Subject>
class ObjectProxyTemplate: public ObjectProxy {
public:
	ObjectProxyTemplate(dpo::common::ObjectId oid)
		: ObjectProxy(oid)
	{ }

	Subject* subject() { return static_cast<Subject*>(subject_); }
	//void set_subject(Subject* subject) { subject_ = subject; } // FIXME: do we need this ???

	Derived* xOpenRO(dpo::stm::client::Transaction* tx);
	Derived* xOpenRO();

private:
	dpo::stm::client::Transaction* tx_;
};


template<class Derived, class Subject>
Derived* ObjectProxyTemplate<Derived, Subject>::xOpenRO(dpo::stm::client::Transaction* tx)
{
	Derived* derived = static_cast<Derived*>(this);
	Subject* subj = derived->subject();
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

//FIXME: VersionManager must be a layer
template<class Derived, class Subject, class VersionManager>
class ObjectProxy: public dpo::cc::client::ObjectProxyTemplate<Derived, Subject>,
                   public VersionManager /* inherit the interface of VersionManager */
{
public:
	ObjectProxy(dpo::common::ObjectId oid)
		: dpo::cc::client::ObjectProxyTemplate<Derived, Subject>(oid),
		  valid_(false)
	{ 
		VersionManager::vSetSubject(dpo::cc::client::ObjectProxyTemplate<Derived, Subject>::subject());
	}

	int vOpen() {
		if (!valid_) {
			return VersionManager::vOpen();
		}
		return 0;
	}
	//vOpenRO(); 
	//vOpenRW();
	
protected:
	bool valid_; // invalid, opened (valid)
};

/*
template<class Derived, class Subject, class VersionManager>
int ObjectProxy<Derived, Subject, VersionManager>::vOpen()
{
}
*/

} // namespace client
} // namespace vm

} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
