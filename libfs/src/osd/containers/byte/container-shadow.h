//! \file
//! Definition of the client side copy-on-write byte container 
//!


#ifndef __STAMNOS_OSD_BYTE_CONTAINER_PROXY_H
#define __STAMNOS_OSD_BYTE_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "bcs/main/common/cdebug.h"
#include "common/util.h"
#include "osd/main/client/session.h"
#include "osd/main/server/session.h"
#include "osd/main/client/rwproxy.h"
#include "osd/containers/byte/common.h"
#include "osd/main/common/obj.h"
#include "osd/main/server/obj.h"
#include "osd/main/server/container.h"

class IntervalTree; // forward declaration

namespace osd {
namespace containers {
namespace client {

typedef ::osd::client::OsdSession OsdSession;

class ByteContainer {
public:
	class VersionManager;

	typedef osd::containers::common::ByteContainer::Object<OsdSession>   Object;
	typedef osd::containers::common::ByteContainer::Region<OsdSession>   Region;
	typedef osd::containers::common::ByteContainer::Slot<OsdSession>     Slot;
	typedef osd::containers::common::ByteContainer::Iterator<OsdSession> Iterator;
	typedef osd::client::rw::ObjectProxy<Object, VersionManager>                 Proxy;
	typedef osd::client::rw::ObjectProxyReference<Object, VersionManager>        Reference;
}; 


class ByteContainer::VersionManager: public osd::vm::client::VersionManager<ByteContainer::Object> {

public:
	VersionManager()
	{
//		printf("\n @ Inside ByteContainer::VersionManager. from src/osd/containers/byte/container-shadow.h");
	}
	int vOpen();
	int vUpdate(OsdSession* session);
	
	int Read(OsdSession* session, char*, uint64_t, uint64_t);
	int Write(OsdSession* session, char*, uint64_t, uint64_t);
	
	int Size(OsdSession* session);
	int Size();

private:
	int ReadImmutable(OsdSession* session, char*, uint64_t, uint64_t);
	int ReadMutable(OsdSession* session, char*, uint64_t, uint64_t);
	int WriteImmutable(OsdSession* session, char*, uint64_t, uint64_t);
	int WriteMutable(OsdSession* session, char*, uint64_t, uint64_t);

	Region*            region_;        // mutable region
	IntervalTree*      intervaltree_;
	uint64_t           size_;
	bool               mutable_;       // directly mutable
};


} // namespace client
} // namespace containers
} // namespace osd


namespace osd {
namespace containers {
namespace server {


class ByteContainer {
public:
	typedef osd::containers::common::ByteContainer::Object< ::osd::server::OsdSession> Object;
	
	class Factory: public ::osd::server::ContainerFactory<ByteContainer> {
	};
}; 


} // namespace server
} // namespace containers
} // namespace osd


#endif // __STAMNOS_OSD_BYTE_CONTAINER_PROXY_H
