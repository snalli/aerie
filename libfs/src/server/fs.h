/**
 * \file fs.h
 *
 * \brief File system descriptor
 *
 */

#ifndef __STAMNOS_PXFS_SERVER_FILE_SYSTEM_DESCRIPTOR_H
#define __STAMNOS_PXFS_SERVER_FILE_SYSTEM_DESCRIPTOR_H

#include "dpo/main/common/obj.h"

namespace server {

// describes a file system instance
class FileSystem {
public:
	virtual dpo::common::ObjectId superblock() = 0;

};

} // namespace server

#endif // __STAMNOS_PXFS_SERVER_FILE_SYSTEM_DESCRIPTOR_H
