//! \file
//! Definition of the client side name container 
//!


#ifndef __STAMNOS_OSD_MAP_CONTAINER_PROXY_H
#define __STAMNOS_OSD_MAP_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "bcs/main/common/cdebug.h"
#include "common/util.h"
#include "osd/containers/name/common.h"
#include "osd/main/client/session.h"
#include "osd/main/server/session.h"
#include "osd/main/client/obj.h"
#include "osd/main/server/obj.h"
#include "osd/main/server/container.h"
#include "osd/main/client/rwproxy.h"

//TODO: Optimistic Read-only proxy: enables faster access for read-only.
// operations. It does an optimistic lookup and puts the result in inode.
// If a mutable version of the mutable exists then this function
// should fail and ask caller to do the normal slow path lookup


namespace osd {
namespace containers {
namespace client {

typedef ::osd::client::OsdSession OsdSession;

class MapContainer {
public:
	class VersionManager;

	typedef osd::containers::common::MapContainer::Object<OsdSession> Object;
	typedef osd::client::rw::ObjectProxy<Object, VersionManager>               Proxy;
	typedef osd::client::rw::ObjectProxyReference<Object, VersionManager>      Reference;
}; 


// In the entry cache we keep negative entries to indicate absence of the entry
// Such entries are marked using FALSE
class MapContainer::VersionManager: public osd::vm::client::VersionManager<MapContainer::Object> {
	typedef google::dense_hash_map<std::string, std::pair<bool, osd::common::ObjectId> > EntryCache;

public:
	VersionManager() 
	{
		entries_.set_empty_key("");
		entries_.set_deleted_key("__#DELETED__KEY#__"); // this must not conflict with a real key
	}

	int vOpen();
	int vUpdate(OsdSession* session);
	
	int Find(OsdSession* session, const char* key, osd::common::ObjectId* oidp);
	int Insert(OsdSession* session, const char* key, osd::common::ObjectId oid);
	int Erase(OsdSession* session, const char* key);

	int Size(OsdSession* session);

private:

	EntryCache entries_;
	int        neg_entries_count_; // number of negative entries in the map entries_
};



} // namespace client
} // namespace containers
} // namespace osd



namespace osd {
namespace containers {
namespace server {


class MapContainer {
public:
	typedef osd::containers::common::MapContainer::Object< ::osd::server::OsdSession> Object;
	
	class Factory: public ::osd::server::ContainerFactory<MapContainer> { };
}; 


} // namespace server
} // namespace containers
} // namespace osd


#endif // __STAMNOS_OSD_MAP_CONTAINER_PROXY_H
