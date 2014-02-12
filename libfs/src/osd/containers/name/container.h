//! \file
//! Definition of the client side name container 
//!


#ifndef __STAMNOS_OSD_NAME_CONTAINER_PROXY_H
#define __STAMNOS_OSD_NAME_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
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

class NameContainer {
public:
	class VersionManager;

	typedef osd::containers::common::NameContainer::Object<OsdSession> Object;
	typedef osd::client::rw::ObjectProxy<Object, VersionManager>               Proxy;
	typedef osd::client::rw::ObjectProxyReference<Object, VersionManager>      Reference;
}; 



// In the entry cache we keep negative entries to indicate absence of the entry
// (in contrast to the DLNC in Solaris and FreeBSD where the negative entry
//  is used as a performance optimization, the negative entry in this system 
//  is necessary for correctness)
// Such entries are marked using FALSE
class NameContainer::VersionManager: public osd::vm::client::VersionManager<NameContainer::Object> {
	struct Shadow {
		Shadow()
			: present(false)
		{ }

		Shadow(bool _present, osd::common::ObjectId _oid, void *_ip) 
			: present(_present),
			  oid(_oid),
			  ip(_ip)
		{ }

		bool                  present;
		osd::common::ObjectId oid;
		void 			*ip;
	};

	typedef google::dense_hash_map<std::string, Shadow> ShadowCache;

public:
	VersionManager() 
	{
	//	printf("\n @ Inside VersionManager. from src/osd/containers/name/container.h");
		entries_.set_empty_key("");
		entries_.set_deleted_key("__#DELETED__KEY#__"); // this must not conflict with a real key
		// insight : Tombstone
	}

	int vOpen();
	int vUpdate(OsdSession* session);
	
	int Find(OsdSession* session, const char* name, osd::common::ObjectId* oidp, void *ip = NULL);
	int Insert(OsdSession* session, const char* name, osd::common::ObjectId oid, void *ip = NULL);
	int Erase(OsdSession* session, const char* name);

	int Size(OsdSession* session);

	struct dentry {
                char key[128];
                uint64_t val;
                struct dentry *next_dentry;
        };
        int return_dentry(void *);

	// do we need these?
	// int Find(::client::Session* session, const char* name, NameContainer::Reference* oref);
	// int Insert(::client::Session* session, const char* name, NameContainer::Reference* oref);
private:

	ShadowCache entries_;
	int         ngv_entries_count_; // number of negative entries in the map entries_
	int         psv_entries_count_; // number of positive entries in the map entries_
};



} // namespace client
} // namespace containers
} // namespace osd



namespace osd {
namespace containers {
namespace server {


class NameContainer {
public:
	typedef osd::containers::common::NameContainer::Object< ::osd::server::OsdSession> Object;
	
	//class Factory: public ::osd::server::ContainerFactory {
	class Factory: public ::osd::server::ContainerFactory<NameContainer> {
	public:
		//::osd::common::Object* Make(::osd::server::OsdSession* session, char* b);
		//int StaticSize();
	};
}; 


} // namespace server
} // namespace containers
} // namespace osd


#endif // __STAMNOS_OSD_NAME_CONTAINER_PROXY_H
