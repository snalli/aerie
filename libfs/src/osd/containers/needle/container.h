//! \file
//! Definition of the needle container 
//!


#ifndef __STAMNOS_OSD_NEEDLE_CONTAINER_PROXY_H
#define __STAMNOS_OSD_NEEDLE_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "bcs/main/common/cdebug.h"
#include "common/util.h"
#include "osd/main/client/session.h"
#include "osd/main/server/session.h"
#include "osd/main/client/rwproxy.h"
#include "osd/containers/needle/common.h"
#include "osd/main/common/obj.h"
#include "osd/main/server/obj.h"
#include "osd/main/server/container.h"

class IntervalTree; // forward declaration

namespace osd {
namespace containers {
namespace client {

typedef ::osd::client::OsdSession OsdSession;


// no proxy object for this container as we don't have metadata 
// use the Object interface directly
class NeedleContainer {
public:
	class VersionManager;

	typedef osd::containers::common::NeedleContainer::Object<OsdSession>   Object;
	typedef osd::client::rw::ObjectProxyReference<Object, VersionManager>  Reference;
};


class NeedleContainer::VersionManager: public osd::vm::client::VersionManager<NeedleContainer::Object> {

public:
	int vOpen();
	int vUpdate(OsdSession* session);
	
	int Read(OsdSession* session, char*, uint64_t, uint64_t);
	int Write(OsdSession* session, char*, uint64_t, uint64_t);
	
	int Size(OsdSession* session);

private:
	uint64_t           size_;
};



} // namespace client
} // namespace containers
} // namespace osd


namespace osd {
namespace containers {
namespace server {


class NeedleContainer {
public:
	typedef osd::containers::common::NeedleContainer::Object< ::osd::server::OsdSession> Object;
	
	class Factory: public ::osd::server::ContainerFactory<NeedleContainer> {
	};
}; 


} // namespace server
} // namespace containers
} // namespace osd


#endif // __STAMNOS_OSD_NEEDLE_CONTAINER_PROXY_H
