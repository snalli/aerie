//! \file
//! Definition of the client side superblock container 
//!


#ifndef __STAMNOS_SSA_SUPER_CONTAINER_PROXY_H
#define __STAMNOS_SSA_SUPER_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/debug.h"
#include "common/util.h"
#include "ssa/containers/assoc/hashtable.h"
#include "ssa/containers/super/common.h"
#include "ssa/main/client/session.h"
#include "ssa/main/server/session.h"
#include "ssa/main/common/obj.h"
#include "ssa/main/client/rwproxy.h"


namespace ssa {
namespace containers {
namespace client {

class SuperContainer {
public:
	class VersionManager;

	typedef ssa::containers::common::SuperContainer::Object< ::client::Session> Object;
	typedef ssa::client::rw::ObjectProxy<Object, VersionManager>                Proxy;
	typedef ssa::client::rw::ObjectProxyReference<Object, VersionManager>       Reference;
}; 


class SuperContainer::VersionManager: public ssa::vm::client::VersionManager<SuperContainer::Object> {
public:
	int vOpen();
	int vUpdate(::client::Session* session);

	ssa::common::ObjectId root(::client::Session* session);
	int set_root(::client::Session* session, ssa::common::ObjectId oid);

private:

};


} // namespace client
} // namespace containers
} // namespace ssa


namespace ssa {
namespace containers {
namespace server {

class SuperContainer {
public:
	typedef ssa::containers::common::SuperContainer::Object< ::ssa::server::SsaSession> Object;
}; 


} // namespace server
} // namespace containers
} // namespace ssa

#endif // __STAMNOS_SSA_NAME_CONTAINER_PROXY_H
