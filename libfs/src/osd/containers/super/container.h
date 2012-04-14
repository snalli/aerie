//! \file
//! Definition of the client side superblock container 
//!


#ifndef __STAMNOS_OSD_SUPER_CONTAINER_PROXY_H
#define __STAMNOS_OSD_SUPER_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "bcs/main/common/cdebug.h"
#include "common/util.h"
#include "osd/containers/map/hashtable.h"
#include "osd/containers/super/common.h"
#include "osd/main/client/session.h"
#include "osd/main/server/session.h"
#include "osd/main/common/obj.h"
#include "osd/main/client/rwproxy.h"


namespace osd {
namespace containers {
namespace client {

typedef ::osd::client::OsdSession OsdSession;

class SuperContainer {
public:
	class VersionManager;

	typedef osd::containers::common::SuperContainer::Object<OsdSession> Object;
	typedef osd::client::rw::ObjectProxy<Object, VersionManager>                Proxy;
	typedef osd::client::rw::ObjectProxyReference<Object, VersionManager>       Reference;
}; 


class SuperContainer::VersionManager: public osd::vm::client::VersionManager<SuperContainer::Object> {
public:
	int vOpen();
	int vUpdate(OsdSession* session);

	osd::common::ObjectId root(OsdSession* session);
	void set_root(OsdSession* session, osd::common::ObjectId oid);

private:

};


} // namespace client
} // namespace containers
} // namespace osd


namespace osd {
namespace containers {
namespace server {

class SuperContainer {
public:
	typedef osd::containers::common::SuperContainer::Object< ::osd::server::OsdSession> Object;
}; 


} // namespace server
} // namespace containers
} // namespace osd

#endif // __STAMNOS_OSD_NAME_CONTAINER_PROXY_H
