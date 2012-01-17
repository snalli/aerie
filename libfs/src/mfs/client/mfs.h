#ifndef __STAMNOS_MFS_CLIENT_MFS_H
#define __STAMNOS_MFS_CLIENT_MFS_H

#include "client/backend.h"
//#include "mfs/sb.h"

namespace mfs {
namespace client {

//client::SuperBlock* CreateSuperBlock(client::Session* session, void* ptr);

void RegisterBackend(::client::FileSystemObjectManager* fsomgr);


} // namespace client
} // namespace mfs

#endif // __STAMNOS_MFS_CLIENT_MFS_H
