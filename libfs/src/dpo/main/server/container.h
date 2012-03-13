#ifndef __STAMNOS_DPO_SERVER_CONTAINER_H
#define __STAMNOS_DPO_SERVER_CONTAINER_H

#include "dpo/main/server/dpo-opaque.h"
#include "dpo/main/server/obj.h"

namespace dpo {
namespace server {

class ContainerAbstractFactory {
public:
	virtual ::dpo::common::Object* Make(::dpo::server::DpoSession* session, char* b) = 0;
	virtual int StaticSize() = 0;
};


template<typename T>
class ContainerFactory: public ContainerAbstractFactory {
public:

	::dpo::common::Object* Make(::dpo::server::DpoSession* session, char* b) {
		return T::Object::Make(session, b);
	}

	int StaticSize() { return sizeof(typename T::Object); }	
};




} // namespace server
} // namespace dpo

#endif // __STAMNOS_DPO_SERVER_CONTAINER_H
