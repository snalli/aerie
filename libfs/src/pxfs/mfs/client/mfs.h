#ifndef __STAMNOS_MFS_CLIENT_MFS_H
#define __STAMNOS_MFS_CLIENT_MFS_H

#include "pxfs/client/backend.h"

namespace mfs {
namespace client {

void RegisterBackend(::client::FileSystemObjectManager* fsomgr);

} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_MFS_H
