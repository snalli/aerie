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
#include "dpo/containers/assoc/hashtable.h"
#include "dpo/containers/name/common.h"
#include "dpo/base/common/obj.h"
#include "dpo/base/client/rwproxy.h"


namespace dpo {
namespace containers {
namespace client {


class SuperContainer {
public:
	class VersionManager;

	typedef dpo::containers::common::SuperContainer::Object< ::client::Session> Object;
	typedef dpo::client::rw::ObjectProxy<Object, VersionManager>                Proxy;
	typedef dpo::client::rw::ObjectProxyReference<Object, VersionManager>       Reference;
}; 


class SuperContainer::VersionManager: public dpo::vm::client::VersionManager<SuperContainer::Object> {
public:
	int vOpen();
	int vUpdate(::client::Session* session);

	dpo::common::ObjectId root(Session* session);
	int set_root(Session* session, dpo::common::ObjectId oid);

private:

};



} // namespace client
} // namespace containers
} // namespace dpo

#endif // __STAMNOS_DPO_NAME_CONTAINER_PROXY_H
