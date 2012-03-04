/**
 * \file fs.h
 *
 * \brief File system descriptor
 *
 */

#ifndef __STAMNOS_PXFS_SERVER_FILE_SYSTEM_DESCRIPTOR_H
#define __STAMNOS_PXFS_SERVER_FILE_SYSTEM_DESCRIPTOR_H

#include "dpo/main/common/obj.h"
#include "dpo/main/server/dpo-opaque.h"
#include "sal/pool/pool.h"

namespace server {

class FileSystemManager; // forward declaration

// describes a file system instance
class FileSystem {
friend class FileSystemManager;
public:
	virtual dpo::common::ObjectId superblock() = 0;

private:
	dpo::server::Dpo* dpo_;
};

} // namespace server

#endif // __STAMNOS_PXFS_SERVER_FILE_SYSTEM_DESCRIPTOR_H
