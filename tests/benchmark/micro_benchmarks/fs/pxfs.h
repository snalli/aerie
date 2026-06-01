#ifndef __STAMNOS_UBENCH_PXFS_H
#define __STAMNOS_UBENCH_PXFS_H

#include "bcs/bcs.h"
#include "common/util.h"
#include "osd/main/client/osd-opaque.h"
#include "micro_benchmarks/fs/fs.h"
#include "micro_benchmarks/main.h"

namespace client
{
extern osd::client::StorageSystem* global_storage_system;
extern Ipc* global_ipc_layer;
} // namespace client

#endif // __STAMNOS_UBENCH_PXFS_H
