// General macros and constants used by the file system

#ifndef __STAMNOS_MFS_CONST_H
#define __STAMNOS_MFS_CONST_H

#include <stdint.h>

namespace mfs {

namespace magic {

const uint32_t kSuperBlock = 0x00010000;
const uint32_t kDirInode   = 0x00010001;
const uint32_t kFileInode  = 0x00010002;

} // namespace magic

} // namespace mfs


#endif // __STAMNOS_MFS_CONST_H
