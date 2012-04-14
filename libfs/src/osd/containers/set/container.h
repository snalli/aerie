#ifndef __STAMNOS_OSD_SET_CONTAINER_H
#define __STAMNOS_OSD_SET_CONTAINER_H

#include "osd/containers/set/common.h"
#include "osd/main/server/session.h"
#include "osd/main/client/session.h"

namespace osd {
namespace containers {
namespace client {

typedef ::osd::client::OsdSession OsdSession;

template<typename T>
class SetContainer {
public:
	typedef typename osd::containers::common::SetContainer<T>::template Object<OsdSession>  Object;
	
};


} // namespace client
} // namespace containers
} // namespace osd



namespace osd {
namespace containers {
namespace server {

template<typename T>
class SetContainer {
public:
	//typedef typename osd::containers::common::SetContainer<T> SetContainerT ;
	//typedef typename osd::containers::common::SetContainer::Object< ::server::Session>  Object;
	//typename SetContainerT::Object< ::server::Session>  Object;

	
	typedef typename osd::containers::common::SetContainer<T>::template Object< ::osd::server::OsdSession>  Object;
	
private:
	//Object* obj_;
};

} // namespace server
} // namespace containers 
} // namespace osd

#endif // __STAMNOS_OSD_SET_CONTAINER_H
