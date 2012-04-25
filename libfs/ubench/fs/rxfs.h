#ifndef __STAMNOS_UBENCH_RXFS_H
#define __STAMNOS_UBENCH_RXFS_H

#include "common/util.h"
#include "bcs/bcs.h"
#include "common/util.h"
#include "ubench/main.h"
#include "ubench/fs/fs.h"

namespace client {
	extern osd::client::StorageSystem* global_storage_system;
	extern Ipc*                        global_ipc_layer;
}

#endif // __STAMNOS_UBENCH_RXFS_H
