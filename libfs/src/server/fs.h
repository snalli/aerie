/**
 * \file fs.h
 *
 * \brief File system descriptor
 *
 */

#ifndef __STAMNOS_PXFS_SERVER_FILE_SYSTEM_DESCRIPTOR_H
#define __STAMNOS_PXFS_SERVER_FILE_SYSTEM_DESCRIPTOR_H

#include "ssa/main/common/obj.h"
#include "ssa/main/server/ssa-opaque.h"
#include "spa/pool/pool.h"

namespace server {

class FileSystemManager; // forward declaration

// describes a file system instance
class FileSystem {
friend class FileSystemManager;
public:
	virtual ssa::common::ObjectId superblock() = 0;

private:
	ssa::server::Dpo* ssa_;
};

} // namespace server

#endif // __STAMNOS_PXFS_SERVER_FILE_SYSTEM_DESCRIPTOR_H
