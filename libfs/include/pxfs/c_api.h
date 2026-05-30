/**
 * PXFS C API - Public Header
 *
 * This is the public C API for the PXFS (POSIX Filesystem) client library.
 *
 * This header provides POSIX-compatible filesystem operations:
 *   - File I/O (open, close, read, write, seek)
 *   - Directory operations (mkdir, rmdir, chdir)
 *   - File metadata (stat, sync)
 *   - Initialization and shutdown
 *
 * Usage:
 *   #include <pxfs/c_api.h>
 *
 *   libfs_init2("server_host");
 *   int fd = libfs_open("/path/to/file", O_RDONLY);
 *   // ... file operations ...
 *   libfs_close(fd);
 *   libfs_shutdown();
 *
 * This API is guaranteed to be stable. Internal headers (client.h, etc.)
 * should not be used by external applications.
 *
 * @see src/pxfs/client/c_api.h for implementation
 */

#ifndef PXFS_PUBLIC_C_API_H
#define PXFS_PUBLIC_C_API_H

/* Include the actual implementation from the source tree */
#include "../../src/pxfs/client/c_api.h"

#endif /* PXFS_PUBLIC_C_API_H */
