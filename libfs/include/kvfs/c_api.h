/**
 * KVFS C API - Public Header
 *
 * This is the public C API for the KVFS (Key-Value Filesystem) client library.
 *
 * This header provides key-value store operations:
 *   - kvfs_put(key, value, size) - Store a key-value pair
 *   - kvfs_get(key, buffer) - Retrieve a value by key
 *   - kvfs_del(key) - Delete a key
 *   - kvfs_mount() - Mount the filesystem
 *   - kvfs_sync() - Synchronize data to persistent storage
 *   - Initialization and shutdown
 *
 * Usage:
 *   #include <kvfs/c_api.h>
 *
 *   kvfs_init2("server_host");
 *   kvfs_mount("/tmp/pool", 0);
 *   kvfs_put("mykey", "myvalue", 7);
 *   // ... more operations ...
 *   kvfs_sync();
 *   kvfs_shutdown();
 *
 * This API is guaranteed to be stable. Internal headers (client.h, table.h, etc.)
 * should not be used by external applications.
 *
 * @see src/kvfs/client/c_api.h for implementation
 */

#ifndef KVFS_PUBLIC_C_API_H
#define KVFS_PUBLIC_C_API_H

/* Include the actual implementation from the source tree */
#include "../../src/kvfs/client/c_api.h"

#endif /* KVFS_PUBLIC_C_API_H */
