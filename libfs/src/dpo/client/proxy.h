#ifndef __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
#define __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H

#include "dpo/common/proxy.h"
#include "dpo/client/hlckmgr.h"
#include "dpo/client/stm.h"

namespace dpo {

// CONCURRENCY CONTROL

namespace cc {

namespace client {

template<class Derived, class Subject>
class ObjectProxy: public dpo::common::ObjectProxyTemplate<Subject> {
public:
	int Lock(lock_protocol::Mode mode) {

	}

	int Lock(ObjectProxy* parent, lock_protocol::Mode mode) {

	}

	Derived* xOpenRO(dpo::stm::client::Transaction* tx);
	Derived* xOpenRO();

private:
	dpo::cc::client::HLock         hlock;
	dpo::stm::client::Transaction* tx_;
};


template<class Derived, class Subject>
Derived* ObjectProxy<Derived, Subject>::xOpenRO(dpo::stm::client::Transaction* tx)
{
	Derived* derived = static_cast<Derived*>(this);
	Subject* subj = derived->subject();
	tx->OpenRO(subj);
	return derived;
}


template<class Derived, class Subject>
Derived* ObjectProxy<Derived, Subject>::xOpenRO()
{
	dpo::stm::client::Transaction* tx = dpo::stm::client::Self();
	return xOpenRO(tx);
}


} // namespace client

} // namespace cc


// VERSION MANAGEMENT

namespace vm {

namespace client {


template<class Derived, class Subject, class VersionManager>
class ObjectProxy: public dpo::cc::client::ObjectProxy<Derived, Subject> {
public:
	//vOpenRO(); 
	//vOpenRW();
	
protected:
	int state_; // invalid, opened (valid)

};


} // namespace client

} // namespace vm

} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_OBJECT_PROXY_H
