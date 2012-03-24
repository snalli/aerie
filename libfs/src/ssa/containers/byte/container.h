//! \file
//! Definition of the client side copy-on-write byte container 
//!


#ifndef __STAMNOS_SSA_BYTE_CONTAINER_PROXY_H
#define __STAMNOS_SSA_BYTE_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "bcs/main/common/cdebug.h"
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

typedef ::ssa::client::SsaSession SsaSession;

class ByteContainer {
public:
	class VersionManager;

	typedef ssa::containers::common::ByteContainer::Object<SsaSession>   Object;
	typedef ssa::containers::common::ByteContainer::Region<SsaSession>   Region;
	typedef ssa::containers::common::ByteContainer::Slot<SsaSession>     Slot;
	typedef ssa::containers::common::ByteContainer::Iterator<SsaSession> Iterator;
	typedef ssa::client::rw::ObjectProxy<Object, VersionManager>                 Proxy;
	typedef ssa::client::rw::ObjectProxyReference<Object, VersionManager>        Reference;
}; 


class ByteContainer::VersionManager: public ssa::vm::client::VersionManager<ByteContainer::Object> {

public:
	int vOpen();
	int vUpdate(SsaSession* session);
	
	int Read(SsaSession* session, char*, uint64_t, uint64_t);
	int Write(SsaSession* session, char*, uint64_t, uint64_t);
	
	int Size(SsaSession* session);

private:
	int ReadImmutable(SsaSession* session, char*, uint64_t, uint64_t);
	int ReadMutable(SsaSession* session, char*, uint64_t, uint64_t);
	int WriteImmutable(SsaSession* session, char*, uint64_t, uint64_t);
	int WriteMutable(SsaSession* session, char*, uint64_t, uint64_t);

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
	typedef ssa::containers::common::ByteContainer::Object< ::ssa::server::SsaSession> Object;
	
	class Factory: public ::ssa::server::ContainerFactory<ByteContainer> {
	};
}; 


} // namespace server
} // namespace containers
} // namespace ssa


#endif // __STAMNOS_SSA_BYTE_CONTAINER_PROXY_H
