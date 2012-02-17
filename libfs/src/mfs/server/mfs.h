#ifndef __STAMNOS_MFS_SERVER_MFS_H
#define __STAMNOS_MFS_SERVER_MFS_H

namespace server {
class FileSystemManager; // forward declaration
} // namespace server


namespace mfs {
namespace server {

void RegisterBackend(::server::FileSystemManager* fsmgr);

} // namespace server
} // namespace mfs

#endif // __STAMNOS_MFS_SERVER_MFS_H
