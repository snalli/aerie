#ifndef __STAMNOS_KVFS_SERVER_FILE_H
#define __STAMNOS_KVFS_SERVER_FILE_H

#include "osd/containers/super/container.h"
#include "osd/containers/needle/container.h"
#include "kvfs/common/types.h"
#include "kvfs/server/session.h"

namespace server {

class File {
public:
	File() { }

	File(InodeNumber ino)
		: oid_(osd::common::ObjectId(ino))
	{
		obj_ = osd::containers::server::NeedleContainer::Object::Load(oid_);
	}

	// TODO: mark persistent object address by ino as allocated and construct inode into ip
	static File* Make(Session* session, InodeNumber ino, File* tp) {
		return new(tp) File(ino);
	}

	static File* Load(Session* session, InodeNumber ino, File* tp) {
		return new(tp) File(ino);
	}

	osd::common::ObjectId oid() { return oid_; }

private:
	osd::common::ObjectId                             oid_;
	osd::containers::server::NeedleContainer::Object* obj_;
};


} // namespace server


#endif // __STAMNOS_KVFS_SERVER_FILE_H

