#ifndef __STAMNOS_MFS_SERVER_FILE_INODE_H
#define __STAMNOS_MFS_SERVER_FILE_INODE_H

#include "ssa/containers/byte/container.h"
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
		ssa::common::ObjectId oid(ino);
		obj_ = ssa::containers::server::ByteContainer::Object::Load(oid);
	}

	// mark persistent object for inode ino as allocated and construct inode into ip
	static FileInode* Make(Session* session, InodeNumber ino, FileInode* ip) {
		ssa::containers::server::NameContainer::Object* obj; 	
		
		return new(ip) FileInode(ino);
	}

	ssa::containers::server::ByteContainer::Object* obj() { return obj_; }
	
	ssa::containers::server::ByteContainer::Object* obj_;
};

} // namespace server

#endif // __STAMNOS_MFS_SERVER_FILE_INODE_H
