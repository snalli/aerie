/**
 * PXFS Server API - Public Header
 *
 * This is the public server API for the PXFS (POSIX Filesystem) library.
 *
 * This header provides server-side filesystem interface for PXFS.
 *
 * Usage:
 *   #include <pxfs/fs.h>
 *
 * This API is guaranteed to be stable. Internal headers (server.h, session.h, etc.)
 * should not be used by external applications.
 *
 * @see src/pxfs/server/fs.h for implementation
 */

#ifndef PXFS_PUBLIC_FS_H
#define PXFS_PUBLIC_FS_H

/* Include the actual implementation from the source tree */
#include "../../src/pxfs/server/fs.h"

#endif /* PXFS_PUBLIC_FS_H */
