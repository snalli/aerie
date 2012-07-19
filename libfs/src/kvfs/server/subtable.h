#ifndef __STAMNOS_KVFS_SERVER_TABLE_H
#define __STAMNOS_KVFS_SERVER_TABLE_H

#include "osd/containers/super/container.h"
#include "osd/containers/name/container.h"
#include "kvfs/common/types.h"
#include "kvfs/server/file.h"
#include "kvfs/server/session.h"

namespace server {

class SubTable {
public:
	SubTable() { }

	SubTable(InodeNumber ino)
		: oid_(osd::common::ObjectId(ino))
	{
		obj_ = osd::containers::server::NameContainer::Object::Load(oid_);
	}

	// TODO: mark persistent object address by ino as allocated and construct inode into ip
	static SubTable* Make(Session* session, InodeNumber ino, SubTable* tp) {
		return new(tp) SubTable(ino);
	}

	static SubTable* Load(Session* session, InodeNumber ino, SubTable* tp) {
		return new(tp) SubTable(ino);
	}

	int Insert(Session* session, const char* key, File* fp);
	int Unlink(Session* session, const char* key);

	osd::common::ObjectId oid() { return oid_; }

private:
	osd::common::ObjectId                           oid_;
	osd::containers::server::NameContainer::Object* obj_;
};


} // namespace server


#endif // __STAMNOS_KVFS_SERVER_TABLE_H

