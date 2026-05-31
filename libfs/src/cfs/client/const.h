#ifndef __STAMNOS_CFS_CLIENT_CONST_H
#define __STAMNOS_CFS_CLIENT_CONST_H

#include "cfs/common/const.h"
#include "common/errno.h"
#include <fcntl.h> // for open flags constant

// namespace scoped constants
namespace client
{

enum
{
    BYPASS_BUFFER = 1
};

namespace limits
{

const int kFileN = 1024;
}

} // namespace client

#endif // __STAMNOS_CFS_CLIENT_CONST_H
