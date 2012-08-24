#ifndef __STAMNOS_KVFS_CLIENT_TABLE_H
#define __STAMNOS_KVFS_CLIENT_TABLE_H

#include "osd/containers/super/container.h"
#include "osd/containers/name/container.h"
#include "osd/containers/array/container.h"
#include "kvfs/client/session.h"

namespace client {

#if KVFS_USE_INDIRECT

typedef osd::containers::client::ArrayContainer<osd::common::ObjectId>::Object ObjectIdArray;

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
	int Erase(::client::Session* session, const char* key);

	int Lock(::client::Session* session, lock_protocol::Mode mode);
	int Unlock(::client::Session* session);

private:
	osd::containers::client::NameContainer::Reference* rw_ref_;
};

#else

class Table {
public:
	static int Load(::client::Session* session, ::osd::common::ObjectId oid, ::client::Table** ipp);
	
	int Put(::client::Session* session, const char* key, const char* src, uint64_t n);
	int Get(::client::Session* session, const char* key, char* dst);
	int Erase(::client::Session* session, const char* key);

	int Lock(::client::Session* session, int name_container, lock_protocol::Mode mode);
	int Unlock(::client::Session* session, int name_container);

private:
	osd::containers::client::NameContainer::Reference* rw_ref_[ARRAY_CONTAINER_SIZE];
};

#endif

} // namespace client


#endif // __STAMNOS_KVFS_CLIENT_TABLE_H

