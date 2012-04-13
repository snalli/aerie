#ifndef __STAMNOS_KVFS_CLIENT_TABLE_H
#define __STAMNOS_KVFS_CLIENT_TABLE_H

#include "osd/containers/super/container.h"
#include "osd/containers/name/container.h"
#include "kvfs/client/session.h"

namespace client {

class Table {
public:
	Table(osd::common::ObjectProxyReference* ref)
		: rw_ref_(static_cast<osd::containers::client::NameContainer::Reference*>(ref))
	{ }


	static int Load(::client::Session* session, ::osd::common::ObjectId oid, ::client::Table** ipp);
	
	osd::containers::client::NameContainer::Reference* rw_ref() {
		return rw_ref_;
	}

	int Put(::client::Session* session, const char* key, const char* src, uint64_t n);
	int Get(::client::Session* session, const char* key, char* dst);

	int Lock(::client::Session* session, lock_protocol::Mode mode);
	int Unlock(::client::Session* session);

private:
	osd::containers::client::NameContainer::Reference* rw_ref_;
};


} // namespace client


#endif // __STAMNOS_KVFS_CLIENT_TABLE_H

