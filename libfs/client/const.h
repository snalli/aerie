#ifndef _CONST_H_DGS189
#define _CONST_H_DGS189

#include "common/errno.h"

// global constants
const int O_CREATE = 1;
//const int O_WRONLY = 1;
//const int O_RDONLY = 1;
//const int O_RDWR = 1;


// namespace scoped constants 
namespace client {

namespace limits {

const int kFileN = 1024;
}


namespace type {

const int kFileInode = 1;
const int kDirInode  = 2;

} // namespace type

} // namespace client

#endif /* _CONST_H_DGS189 */
