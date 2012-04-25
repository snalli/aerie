#ifndef __STAMNOS_RXFS_SERVER_FILE_INODE_H
#define __STAMNOS_RXFS_SERVER_FILE_INODE_H

#include "osd/containers/byte/container.h"
#include "rxfs/client/inode.h"

namespace rxfs {
namespace client {

class Session; // forward declaration

class FileInode: public InodeT<FileInode> {
public:

	FileInode()
	{ }

	FileInode(InodeNumber ino)
		: InodeT<FileInode>(ino, kFileInode)
	{ 
		osd::common::ObjectId oid(ino);
		obj_ = osd::containers::client::ByteContainer::Object::Load(oid);
	}

	static FileInode* Make(Session* session, FileInode* ip) {
		osd::common::ObjectId oid;
		int                   ret;

		if ((ret = session->salloc()->AllocateContainer(session, 0, osd::containers::T_BYTE_CONTAINER, &oid)) < 0) {
			return NULL;
		}
		return new(ip) FileInode(oid.u64());
	}

	int Write(Session* session, void* src, int count, int offset) {
		return obj_->Write(session, (char*) src, offset, count);
	}

	int Read(Session* session, void* dst, int count, int offset) {
		return obj_->Read(session, (char*) dst, offset, count);
	}

	osd::containers::client::ByteContainer::Object* obj() { return obj_; }
	
	osd::containers::client::ByteContainer::Object* obj_;
};

} // namespace client
} // namespace rxfs

#endif // __STAMNOS_RXFS_CLIENT_FILE_INODE_H
