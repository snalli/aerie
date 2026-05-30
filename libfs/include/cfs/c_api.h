/**
 * CFS C API - Public Header
 *
 * This is the public C API for the CFS (Cache Filesystem) client library.
 *
 * This header provides POSIX-like filesystem operations with caching:
 *   - File I/O (open, close, read, write, seek)
 *   - Directory operations (mkdir, rmdir, chdir)
 *   - File metadata (stat, sync)
 *   - Initialization and shutdown
 *
 * CFS provides a caching layer on top of backend storage for improved performance.
 *
 * Usage:
 *   #include <cfs/c_api.h>
 *
 *   libfs_init2("server_host");
 *   int fd = libfs_open("/path/to/file", O_RDONLY);
 *   // ... file operations ...
 *   libfs_close(fd);
 *   libfs_shutdown();
 *
 * This API is guaranteed to be stable. Internal headers (client.h, file.h, etc.)
 * should not be used by external applications.
 *
 * @see src/cfs/client/c_api.h for implementation
 */

#ifndef CFS_PUBLIC_C_API_H
#define CFS_PUBLIC_C_API_H

/* Include the actual implementation from the source tree */
#include "../../src/cfs/client/c_api.h"

#endif /* CFS_PUBLIC_C_API_H */
