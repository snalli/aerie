//! \file
//! Definition of the client side name container 
//!


#ifndef __STAMNOS_DPO_NAME_CONTAINER_PROXY_H
#define __STAMNOS_DPO_NAME_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/debug.h"
#include "common/util.h"
#include "client/session.h"
#include "dpo/containers/name/common.h"
#include "dpo/base/common/obj.h"
#include "dpo/base/client/rwproxy.h"

//TODO: Optimistic Read-only proxy: enables faster access for read-only.
// operations. It does an optimistic lookup and puts the result in inode.
// If a mutable version of the mutable exists then this function
// should fail and ask caller to do the normal slow path lookup


namespace dpo {
namespace containers {
namespace client {


class NameContainer {
public:
	class VersionManager;

	typedef dpo::containers::common::NameContainer::Object< ::client::Session> Object;
	typedef dpo::client::rw::ObjectProxy<Object, VersionManager>               Proxy;
	typedef dpo::client::rw::ObjectProxyReference<Object, VersionManager>      Reference;
}; 


// In the entry cache we keep negative entries to indicate absence of the entry
// (in contrast to the DLNC in Solaris and FreeBSD where the negative entry
//  is used as a performance optimization, the negative entry in this system 
//  is necessary for correctness)
// Such entries are marked using FALSE
class NameContainer::VersionManager: public dpo::vm::client::VersionManager<NameContainer::Object> {
	typedef google::dense_hash_map<std::string, std::pair<bool, dpo::common::ObjectId> > EntryCache;

public:
	int vOpen();
	int vUpdate(::client::Session* session);
	
	int Find(::client::Session* session, const char* name, dpo::common::ObjectId* oidp);
	int Insert(::client::Session* session, const char* name, dpo::common::ObjectId oid);
	int Erase(::client::Session* session, const char* name);

	int Size(::client::Session* session);

	// do we need these?
	// int Find(::client::Session* session, const char* name, NameContainer::Reference* oref);
	// int Insert(::client::Session* session, const char* name, NameContainer::Reference* oref);
private:

	EntryCache entries_;
	int        neg_entries_count_; // number of negative entries in the map entries_
};



} // namespace client
} // namespace containers
} // namespace dpo

#endif // __STAMNOS_DPO_NAME_CONTAINER_PROXY_H
