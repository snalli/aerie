#ifndef __STAMNOS_SSA_SERVER_CONTAINER_H
#define __STAMNOS_SSA_SERVER_CONTAINER_H

#include "ssa/main/server/ssa-opaque.h"
#include "ssa/main/server/obj.h"

namespace ssa {
namespace server {


// For use when type is not known at static time but provided during runtime

class ContainerAbstractFactory {
public:
	virtual ::ssa::common::Object* Make(::ssa::server::SsaSession* session, char* b) = 0;
	virtual int StaticSize() = 0;
};


template<typename T>
class ContainerFactory: public ContainerAbstractFactory {
public:

	::ssa::common::Object* Make(::ssa::server::SsaSession* session, char* b) {
		return T::Object::Make(session, b);
	}

	int StaticSize() { return sizeof(typename T::Object); }	
};




} // namespace server
} // namespace ssa

#endif // __STAMNOS_SSA_SERVER_CONTAINER_H
