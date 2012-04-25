#ifndef __STAMNOS_RXFS_CLIENT_CONST_H
#define __STAMNOS_RXFS_CLIENT_CONST_H

#include <fcntl.h> // for open flags constant
#include "rxfs/common/const.h"
#include "common/errno.h"

// namespace scoped constants 
namespace rxfs {
namespace client {


// global constants
// PORTABILITY ISSUE: ensure that across platforms, open flags have the same 
// values as the ones used by the kernel (in /usr/include/bits/fcntl.h)
//const int O_RDONLY  = 0x0000;
//const int O_WRONLY  = 0x0001;
//const int O_RDWR    = 0x0002;
//const int O_CREAT   = 0x0100;
//const int O_EXCL    = 0x0200;
//const int O_TRUNC   = 0x1000;
//const int O_APPEND  = 0x2000;


enum {
	BYPASS_BUFFER = 1
};

namespace limits {

const int kFileN = 1024;
}


} // namespace client
} // namespace rxfs

#endif // __STAMNOS_RXFS_CLIENT_CONST_H
