#ifndef __STAMNOS_MFS_SERVER_FILE_INODE_H
#define __STAMNOS_MFS_SERVER_FILE_INODE_H

#include "osd/containers/byte/container.h"
#include "pxfs/server/inode.h"

namespace server {

class Session; // forward declaration

class FileInode: public InodeT<FileInode> {
public:

	FileInode()
	{ }

	FileInode(InodeNumber ino)
		: InodeT<FileInode>(ino, kFileInode)
	{ 
		osd::common::ObjectId oid(ino);
		obj_ = osd::containers::server::ByteContainer::Object::Load(oid);
	}

	// mark persistent object for inode ino as allocated and construct inode into ip
	static FileInode* Make(Session* session, InodeNumber ino, FileInode* ip) {
		return new(ip) FileInode(ino);
	}

	osd::containers::server::ByteContainer::Object* obj() { return obj_; }
	
	osd::containers::server::ByteContainer::Object* obj_;
};

} // namespace server

#endif // __STAMNOS_MFS_SERVER_FILE_INODE_H
