// General macros and constants used by the file system

#ifndef _MFS_CONST_H_ADF102
#define _MFS_CONST_H_ADF102

namespace mfs {

namespace magic {

const uint32_t kSuperBlock = 0x00010000;
const uint32_t kDirInode   = 0x00010001;
const uint32_t kFileInode  = 0x00010002;

} // namespace magic

} // namespace mfs


#endif  /* _MFS_CONST_H_ADF102 */
