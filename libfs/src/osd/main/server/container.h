#ifndef __STAMNOS_OSD_SERVER_CONTAINER_H
#define __STAMNOS_OSD_SERVER_CONTAINER_H

#include "osd/main/server/osd-opaque.h"
#include "osd/main/server/obj.h"

namespace osd {
namespace server {


// For use when type is not known at static time but provided during runtime

class ContainerAbstractFactory {
public:
	virtual ::osd::common::Object* Make(::osd::server::OsdSession* session, char* b) = 0;
	virtual int StaticSize() = 0;
};


template<typename T>
class ContainerFactory: public ContainerAbstractFactory {
public:

	::osd::common::Object* Make(::osd::server::OsdSession* session, char* b) {
		return T::Object::Make(session, b);
	}

	int StaticSize() { return sizeof(typename T::Object); }	
};




} // namespace server
} // namespace osd

#endif // __STAMNOS_OSD_SERVER_CONTAINER_H
