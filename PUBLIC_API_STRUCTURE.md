# Public API Structure - Clean Separation of Public vs Private Headers

## Overview

This document describes the unified public header structure implemented for Aerie's filesystem libraries.

---

## Directory Structure

```
aerie/libfs/
├── include/                          # ← PUBLIC HEADERS (unified location)
│   ├── pxfs/
│   │   ├── c_api.h                   # Client API (includes from src/)
│   │   └── fs.h                      # Server API (includes from src/)
│   ├── kvfs/
│   │   ├── c_api.h                   # Client API (put/get/del)
│   │   └── fs.h                      # Server API
│   └── cfs/
│       ├── c_api.h                   # Client API
│       └── fs.h                      # Server API
│
├── src/
│   ├── pxfs/
│   │   ├── client/                   # ← IMPLEMENTATION (private)
│   │   │   ├── c_api.h               # Actual implementation
│   │   │   ├── client.h              # Private headers
│   │   │   ├── file.h
│   │   │   ├── inode.h
│   │   │   └── ... (other internals)
│   │   ├── server/                   # ← IMPLEMENTATION (private)
│   │   │   ├── fs.h                  # Actual implementation
│   │   │   ├── server.h              # Private headers
│   │   │   ├── session.h
│   │   │   └── ... (other internals)
│   │   └── CMakeLists.txt
│   │
│   ├── kvfs/
│   │   ├── client/                   # ← IMPLEMENTATION (private)
│   │   │   ├── c_api.h
│   │   │   ├── client.h
│   │   │   ├── table.h
│   │   │   └── ... (other internals)
│   │   ├── server/                   # ← IMPLEMENTATION (private)
│   │   │   ├── fs.h
│   │   │   ├── server.h
│   │   │   ├── subtable.h
│   │   │   └── ... (other internals)
│   │   └── CMakeLists.txt
│   │
│   └── cfs/
│       ├── client/                   # ← IMPLEMENTATION (private)
│       ├── server/                   # ← IMPLEMENTATION (private)
│       └── CMakeLists.txt
│
└── bench/ubench/
    └── (tests using public headers)
```

---

## Public Headers (in `include/`)

### PXFS Public API

```c
#include <pxfs/c_api.h>      // Client API (POSIX operations)
#include <pxfs/fs.h>         // Server API

// Client usage
libfs_init2("server_host");
int fd = libfs_open("/path/to/file", O_RDONLY);
libfs_read(fd, buffer, size);
libfs_close(fd);
libfs_shutdown();
```

**What's included:**
- `include/pxfs/c_api.h` → wraps `src/pxfs/client/c_api.h`
- `include/pxfs/fs.h` → wraps `src/pxfs/server/fs.h`

### KVFS Public API

```c
#include <kvfs/c_api.h>      // Client API (key-value operations)
#include <kvfs/fs.h>         // Server API

// Client usage
kvfs_init2("server_host");
kvfs_mount("/tmp/pool", 0);
kvfs_put("key1", "value1", 6);
char buffer[256];
kvfs_get("key1", buffer);
kvfs_sync();
kvfs_shutdown();
```

**What's included:**
- `include/kvfs/c_api.h` → wraps `src/kvfs/client/c_api.h`
- `include/kvfs/fs.h` → wraps `src/kvfs/server/fs.h`

### CFS Public API

```c
#include <cfs/c_api.h>       // Client API (POSIX-like operations)
#include <cfs/fs.h>          // Server API

// Client usage (similar to PXFS)
libfs_init2("server_host");
int fd = libfs_open("/path/to/file", O_RDONLY);
libfs_read(fd, buffer, size);
libfs_close(fd);
libfs_shutdown();
```

**What's included:**
- `include/cfs/c_api.h` → wraps `src/cfs/client/c_api.h`
- `include/cfs/fs.h` → wraps `src/cfs/server/fs.h`

---

## Private Headers (in `src/`)

These headers are implementation details and should NOT be used by external code:

### PXFS Private Headers

```
src/pxfs/client/
├── c_api.h                 # Implementation of public API
├── client.h                # Client state management (PRIVATE)
├── file.h                  # File descriptor handling (PRIVATE)
├── inode.h                 # Inode structures (PRIVATE)
├── mpinode.h               # Multi-path inode (PRIVATE)
├── namespace.h             # Namespace operations (PRIVATE)
├── fsomgr.h                # Filesystem object manager (PRIVATE)
├── cache.h                 # Caching mechanism (PRIVATE)
└── ...

src/pxfs/server/
├── fs.h                    # Implementation of public API
├── server.h                # Server implementation (PRIVATE)
├── session.h               # Client session management (PRIVATE)
├── publisher.h             # Message publishing (PRIVATE)
└── ...
```

### KVFS Private Headers

```
src/kvfs/client/
├── c_api.h                 # Implementation of public API
├── client.h                # Client connection management (PRIVATE)
├── table.h                 # Key-value table interface (PRIVATE)
├── sb.h                    # Superblock management (PRIVATE)
└── ...

src/kvfs/server/
├── fs.h                    # Implementation of public API
├── server.h                # Server implementation (PRIVATE)
├── session.h               # Client session management (PRIVATE)
├── subtable.h              # Table subdivisions (PRIVATE)
├── publisher.h             # Message publishing (PRIVATE)
└── ...
```

---

## CMake Integration

### Using Public Headers in Your Code

```cmake
# Linking against PXFS client
add_executable(my_pxfs_app main.cc)
target_link_libraries(my_pxfs_app PRIVATE pxfsc)

# CMake automatically includes: ${CMAKE_SOURCE_DIR}/include
# So you can use: #include <pxfs/c_api.h>
```

### CMakeLists Configuration

Each library is configured to expose public headers:

```cmake
target_include_directories(pxfsc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>   # ← Public headers
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/client>  # ← Implementation
    $<INSTALL_INTERFACE:include>
)
```

**Why two include directories?**
1. `${CMAKE_SOURCE_DIR}/include` - Points to public wrapper headers
2. `${CMAKE_CURRENT_SOURCE_DIR}/client` - Points to implementation (for internal includes)

---

## Benefits of This Structure

### ✅ Clear API Boundaries
- Users know exactly which headers are stable/public
- Implementation headers are clearly separate

### ✅ Easy Refactoring
- Internal implementation can be reorganized without affecting users
- Public API remains stable

### ✅ Documentation
- Public headers have clear comments about stability
- Users can't accidentally use internal APIs

### ✅ Distribution
- Public headers can be easily copied for distribution
- Clear what needs to be installed with the library

### ✅ IDE/Tooling Support
- Public headers appear first in include paths
- Better autocomplete and documentation

---

## How Public Headers Work

### Public Header Example: `include/pxfs/c_api.h`

```c
/**
 * PXFS C API - Public Header
 *
 * This is the stable, public API for PXFS filesystem.
 * Applications should include this header.
 *
 * @see src/pxfs/client/c_api.h for implementation
 */

#ifndef PXFS_PUBLIC_C_API_H
#define PXFS_PUBLIC_C_API_H

/* Include the actual implementation from the source tree */
#include "../../src/pxfs/client/c_api.h"

#endif /* PXFS_PUBLIC_C_API_H */
```

**How it works:**
1. User includes: `#include <pxfs/c_api.h>`
2. This includes the wrapper at: `include/pxfs/c_api.h`
3. The wrapper includes the actual implementation: `src/pxfs/client/c_api.h`
4. User gets the API without needing to know source structure

---

## Migration Guide

### Old Way (Direct Source Includes)

```c
#include "pxfs/client/c_api.h"     // ❌ Exposes internal structure
#include "kvfs/client/c_api.h"
#include "cfs/client/c_api.h"
```

### New Way (Public Headers)

```c
#include <pxfs/c_api.h>     // ✅ Clean, stable API
#include <kvfs/c_api.h>
#include <cfs/c_api.h>
```

**What changed:**
- Include paths changed (cleaner)
- No code changes needed (headers are included transitively)
- Better isolation of public vs private APIs

---

## File Structure Summary

| Location | Type | Usage | Stability |
|----------|------|-------|-----------|
| `include/pxfs/c_api.h` | Public Wrapper | `#include <pxfs/c_api.h>` | ✅ Stable |
| `include/pxfs/fs.h` | Public Wrapper | `#include <pxfs/fs.h>` | ✅ Stable |
| `src/pxfs/client/c_api.h` | Implementation | Internal only | ⚠️ May change |
| `src/pxfs/client/client.h` | Private | Not for external use | ❌ Internal |
| `src/pxfs/server/fs.h` | Implementation | Internal only | ⚠️ May change |
| `src/pxfs/server/server.h` | Private | Not for external use | ❌ Internal |

---

## Optional Future Improvements

### 1. Install Rules
Add CMake install rules to package public headers:

```cmake
install(FILES include/pxfs/c_api.h DESTINATION include/pxfs)
install(FILES include/kvfs/c_api.h DESTINATION include/kvfs)
install(TARGETS pxfsc pxfss kvfsc kvfss DESTINATION lib)
```

### 2. Version Information
Add library versioning:

```cmake
set_target_properties(pxfsc PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)
```

### 3. Symbol Visibility
Use compiler flags to hide internal symbols:

```cmake
set_target_properties(pxfsc PROPERTIES
    COMPILE_FLAGS "-fvisibility=hidden"
)
```

---

## Summary

**Before:** Unclear public vs private, users could accidentally depend on internals
**After:** Clear public APIs in `include/`, implementation hidden in `src/`

**Result:**
- ✅ Stable, documented public APIs
- ✅ Clean separation of concerns
- ✅ Easy to refactor internals
- ✅ Better for distribution/installation
- ✅ Improved IDE/tooling support

---

**Implementation Completed**: 2026-05-30
**Status**: ✅ Phase 1, 2, & 3 Complete
**Next Step**: Optional install rules or version numbering
