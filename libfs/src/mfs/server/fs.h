/**
 * \file fs.h
 *
 * \brief File system descriptor
 *
 */

#ifndef __STAMNOS_PXFS_SERVER_MFS_FILE_SYSTEM_DESCRIPTOR_H
#define __STAMNOS_PXFS_SERVER_MFS_FILE_SYSTEM_DESCRIPTOR_H

#include "server/fs.h"
#include "dpo/main/common/obj.h"

namespace mfs {
namespace server {

// describes a file system instance
class FileSystem: public ::server::FileSystem {
public:
	FileSystem(dpo::common::ObjectId super);
	dpo::common::ObjectId superblock() { return superblock_; }

private:
	dpo::common::ObjectId superblock_;

};

} // namespace mfs
} // namespace server

#endif // __STAMNOS_PXFS_SERVER_MFS_FILE_SYSTEM_DESCRIPTOR_H
