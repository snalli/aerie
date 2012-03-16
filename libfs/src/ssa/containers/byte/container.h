//! \file
//! Definition of the client side copy-on-write byte container 
//!


#ifndef __STAMNOS_SSA_BYTE_CONTAINER_PROXY_H
#define __STAMNOS_SSA_BYTE_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "common/debug.h"
#include "common/util.h"
#include "ssa/main/client/session.h"
#include "ssa/main/server/session.h"
#include "ssa/main/client/rwproxy.h"
#include "ssa/containers/byte/common.h"
#include "ssa/main/common/obj.h"
#include "ssa/main/server/obj.h"
#include "ssa/main/server/container.h"

class IntervalTree; // forward declaration

namespace ssa {
namespace containers {
namespace client {


class ByteContainer {
public:
	class VersionManager;

	typedef ssa::containers::common::ByteContainer::Object< ::client::Session>   Object;
	typedef ssa::containers::common::ByteContainer::Region< ::client::Session>   Region;
	typedef ssa::containers::common::ByteContainer::Slot< ::client::Session>     Slot;
	typedef ssa::containers::common::ByteContainer::Iterator< ::client::Session> Iterator;
	typedef ssa::client::rw::ObjectProxy<Object, VersionManager>                 Proxy;
	typedef ssa::client::rw::ObjectProxyReference<Object, VersionManager>        Reference;
}; 


class ByteContainer::VersionManager: public ssa::vm::client::VersionManager<ByteContainer::Object> {

public:
	int vOpen();
	int vUpdate(::client::Session* session);
	
	int Read(::client::Session* session, char*, uint64_t, uint64_t);
	int Write(::client::Session* session, char*, uint64_t, uint64_t);
	
	int Size(::client::Session* session);

private:
	int ReadImmutable(::client::Session* session, char*, uint64_t, uint64_t);
	int ReadMutable(::client::Session* session, char*, uint64_t, uint64_t);
	int WriteImmutable(::client::Session* session, char*, uint64_t, uint64_t);
	int WriteMutable(::client::Session* session, char*, uint64_t, uint64_t);

	Region*            region_;        // mutable region
	IntervalTree*      intervaltree_;
	uint64_t           size_;
	bool               mutable_;       // directly mutable
};


} // namespace client
} // namespace containers
} // namespace ssa


namespace ssa {
namespace containers {
namespace server {


class ByteContainer {
public:
	typedef ssa::containers::common::ByteContainer::Object< ::ssa::server::DpoSession> Object;
	
	class Factory: public ::ssa::server::ContainerFactory<ByteContainer> {
	};
}; 


} // namespace server
} // namespace containers
} // namespace ssa


#endif // __STAMNOS_SSA_BYTE_CONTAINER_PROXY_H
