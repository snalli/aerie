#ifndef __STAMNOS_PXFS_SERVER_INODE_H
#define __STAMNOS_PXFS_SERVER_INODE_H

#include "ssa/containers/name/container.h"
#include "ssa/containers/byte/container.h"
#include "ssa/main/common/obj.h"
#include "pxfs/server/const.h"
#include "pxfs/server/session.h"
#include "pxfs/common/types.h"

namespace server {

class Inode {
public:
	Inode()
		: oid_(ssa::common::ObjectId(0)),
		  ino_(0),
		  type_(0)
	{ }
	
	Inode(InodeNumber ino, int type)
		: oid_(ssa::common::ObjectId(ino)),
		  ino_(ino),
		  type_(type)
	{ }
	
	static int type(InodeNumber ino) {
		ssa::common::ObjectId oid(ino);
		ssa::common::Object*  obj = ssa::common::Object::Load(oid);
		switch (obj->type()) {
			case ssa::containers::T_NAME_CONTAINER:
				return kDirInode;
			case ssa::containers::T_BYTE_CONTAINER:
				return kFileInode;
		}
		return -1;
	}

	ssa::common::ObjectId oid() { return oid_; }

	ssa::common::ObjectId oid_;
	InodeNumber           ino_;
	int                   type_;
};


template<typename T>
class InodeT: public Inode {
public:
	InodeT()
	{ }

	InodeT(InodeNumber ino, int type)
		: Inode(ino, type)
	{ }

	// construct inode over existing persistent persistent object into ip
	static T* Load(Session* session, InodeNumber ino, T* ip) {
		return new(ip) T(ino);
	}
	
};


} // namespace server

#endif // __STAMNOS_PXFS_SERVER_INODE_H
