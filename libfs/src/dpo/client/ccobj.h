#ifndef __STAMNOS_DPO_CLIENT_CC_OBJECT_H
#define __STAMNOS_DPO_CLIENT_CC_OBJECT_H

#include "dpo/client/hlckmgr.h"

namespace dpo {

namespace cc {

namespace client {

template<class Derived, class Subject>
class CCObject {
public:
	int Lock() {

	}

	int Lock(Object* parent) {

	}

	Derived* xOpenRO(Transaction* tx);
	Derived* xOpenRO();

private:
	HLock        lock;
	Transaction* tx_;


};


template<class Derived, class Subject>
Derived* Object<Derived, Subject>::xOpenRO(Transaction* tx)
{
	Derived* derived = static_cast<Derived*>(this);
	Object* subj = derived->subject();
	tx->OpenRO(subj);
	return proxy;
}


template<class Derived, class Subject>
Derived* Object<Derived, Subject>::xOpenRO()
{
	Transaction* tx = Self();
	return xOpenRO(tx);
}



} // namespace client

} // namespace cc

} // namespace dpo

#endif // __STAMNOS_DPO_CLIENT_CC_OBJECT_H
