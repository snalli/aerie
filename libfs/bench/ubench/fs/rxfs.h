#ifndef __STAMNOS_UBENCH_RXFS_H
#define __STAMNOS_UBENCH_RXFS_H

#include "bcs/bcs.h"
#include "common/util.h"
#include "ubench/fs/fs.h"
#include "ubench/main.h"

namespace client
{
extern osd::client::StorageSystem* global_storage_system;
extern Ipc* global_ipc_layer;
} // namespace client

#endif // __STAMNOS_UBENCH_RXFS_H
