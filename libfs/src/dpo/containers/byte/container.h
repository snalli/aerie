//! \file
//! Definition of the client side copy-on-write byte container 
//!


#ifndef __STAMNOS_DPO_BYTE_CONTAINER_PROXY_H
#define __STAMNOS_DPO_BYTE_CONTAINER_PROXY_H

#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include "common/debug.h"
#include "common/util.h"
#include "client/session.h"
#include "dpo/base/client/rwproxy.h"
#include "dpo/containers/byte/common.h"
#include "dpo/base/common/obj.h"

class IntervalTree; // forward declaration

namespace dpo {
namespace containers {
namespace client {


class ByteContainer {
public:
	class VersionManager;

	typedef dpo::containers::common::ByteContainer::Object< ::client::Session>   Object;
	typedef dpo::containers::common::ByteContainer::Region< ::client::Session>   Region;
	typedef dpo::containers::common::ByteContainer::Slot< ::client::Session>     Slot;
	typedef dpo::containers::common::ByteContainer::Iterator< ::client::Session> Iterator;
	typedef dpo::client::rw::ObjectProxy<Object, VersionManager>                 Proxy;
	typedef dpo::client::rw::ObjectProxyReference<Object, VersionManager>        Reference;
}; 


class ByteContainer::VersionManager: public dpo::vm::client::VersionManager<ByteContainer::Object> {

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
} // namespace dpo

#endif // __STAMNOS_DPO_BYTE_CONTAINER_PROXY_H
