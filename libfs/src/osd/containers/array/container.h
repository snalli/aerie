#ifndef __STAMNOS_OSD_ARRAY_CONTAINER_H
#define __STAMNOS_OSD_ARRAY_CONTAINER_H

#include "osd/containers/array/common.h"
#include "osd/main/server/session.h"
#include "osd/main/client/session.h"

namespace osd {
namespace containers {
namespace client {

typedef ::osd::client::OsdSession OsdSession;

template<typename T>
class ArrayContainer {
public:
	typedef typename osd::containers::common::ArrayContainer<T>::template Object<OsdSession>  Object;
	
};


} // namespace client
} // namespace containers
} // namespace osd



namespace osd {
namespace containers {
namespace server {

template<typename T>
class ArrayContainer {
public:
	typedef typename osd::containers::common::ArrayContainer<T>::template Object< ::osd::server::OsdSession>  Object;
	
private:
};

} // namespace server
} // namespace containers 
} // namespace osd

#endif // __STAMNOS_OSD_ARRAY_CONTAINER_H
