# Aerie Public API Organization

## Overview

This document describes the public APIs and their organization after the refactoring.

---

## Naming Consistency Improvements

### Phase 1: PXFS Library Renaming ✅ COMPLETED

**Old Names:**
```cmake
fsc   → PXFS client library
fss   → PXFS server library
```

**New Names:**
```cmake
pxfsc → PXFS client library
pxfss → PXFS server library
```

**Benefits:**
- ✅ Consistent with kvfsc/kvfss, cfsc/cfss naming
- ✅ Immediately clear which filesystem it belongs to
- ✅ Easier to grep and search for pxfs-related code
- ✅ Better for documentation and tutorials

**Changes Made:**
- Updated: `src/pxfs/CMakeLists.txt`
- Updated: `bench/ubench/CMakeLists.txt`
- Renamed library references: `fsc` → `pxfsc`, `fss` → `pxfss`

---

## API Organization by Filesystem

### PXFS (POSIX Filesystem)

```
pxfsc (Client Library)
├── Public API: include/pxfs/c_api.h
│   ├── libfs_init()
│   ├── libfs_open()
│   ├── libfs_read()
│   ├── libfs_write()
│   ├── libfs_close()
│   └── ... (all POSIX operations)
│
└── Internal Headers (not for external use):
    ├── client.h (client state management)
    ├── file.h (file descriptor handling)
    ├── inode.h (inode structures)
    ├── namespace.h (namespace operations)
    └── fsomgr.h (filesystem object manager)

pxfss (Server Library)
├── Public API: include/pxfs/fs.h
│   └── Server-side filesystem interface
│
└── Internal Headers (not for external use):
    ├── server.h (server implementation)
    ├── session.h (client session management)
    └── ... (other internals)

Executables:
├── pxfs_server → uses pxfss + infrastructure
├── pxfs_client → uses pxfsc (debug/test)
└── pxfs_mkfs → filesystem formatting tool
```

### KVFS (Key-Value Filesystem)

```
kvfsc (Client Library)
├── Public API: include/kvfs/c_api.h
│   ├── kvfs_put()
│   ├── kvfs_get()
│   ├── kvfs_del()
│   ├── kvfs_mount()
│   └── kvfs_sync()
│
└── Internal Headers (not for external use):
    ├── client.h (client connection management)
    ├── table.h (key-value table interface)
    ├── sb.h (superblock management)
    └── ... (other internals)

kvfss (Server Library)
├── Public API: include/kvfs/fs.h
│   └── Server-side filesystem interface
│
└── Internal Headers (not for external use):
    ├── server.h (server implementation)
    ├── session.h (client session management)
    ├── subtable.h (table subdivisions)
    └── ... (other internals)

Executables:
├── kvfs_server → uses kvfss + infrastructure
└── kvfs_mkfs → filesystem formatting tool
```

### CFS (Cache Filesystem)

```
cfsc (Client Library)
├── Public API: include/cfs/c_api.h
│   ├── libfs_init()
│   ├── libfs_open()
│   ├── libfs_read()
│   ├── libfs_write()
│   └── ... (POSIX-like operations)
│
└── Internal Headers (not for external use):
    ├── client.h (client state management)
    ├── file.h (file descriptor handling)
    └── ... (other internals)

cfss (Server Library)
├── Public API: include/cfs/fs.h
│   └── Server-side filesystem interface
│
└── Internal Headers (not for external use):
    ├── server.h (server implementation)
    ├── session.h (client session management)
    ├── namespace.h (namespace operations)
    └── ... (other internals)

Executables:
├── cfs_server → uses cfss + infrastructure
├── cfs_client → uses cfsc (debug/test)
└── cfs_mkfs → filesystem formatting tool
```

---

## Shared Infrastructure Libraries

All filesystems depend on these lower-level libraries:

### SCM (Storage Class Memory)
```
libscm.so
├── Public API: include/scm/scm.h
│   ├── scm_pool_create()
│   ├── scm_pool_open()
│   ├── scm_alloc()
│   ├── scm_free()
│   └── ... (memory management)
│
└── Configuration: SCM_POOL (kernel or user mode)
```

### OSD (Object Storage Device)

```
osdclt (Client Library)
├── Uses: bcsclt, scm, common
└── Internal API for filesystem clients

osdsrv (Server Library)
├── Uses: bcssrv, scm, common
└── Internal API for filesystem servers
```

### BCS (Backend Communication System)

```
bcsclt (Client Library)
├── RPC client implementation
├── Uses: scm, common
└── Supports: RPC backends (net, fast, fast-two)

bcssrv (Server Library)
├── RPC server implementation
├── Uses: scm, common
└── Supports: RPC backends (net, fast, fast-two)
```

### Common Utilities

```
libcommon.so
├── Logging and debugging utilities
├── Memory allocation helpers
├── System utilities
└── Error handling
```

---

## Public vs Private Headers

### Public Headers (Stable API)

These headers are guaranteed to be stable and are safe for external use:

| Filesystem | Public Client Header | Public Server Header |
|-----------|----------------------|----------------------|
| **PXFS** | client/c_api.h | server/fs.h |
| **KVFS** | client/c_api.h | server/fs.h |
| **CFS** | client/c_api.h | server/fs.h |

### Private Headers (Internal Implementation)

These headers are implementation details and should NOT be used by external code:

| Filesystem | Private Headers |
|-----------|------------------|
| **PXFS Client** | client.h, file.h, inode.h, namespace.h, fsomgr.h, cache.h, ... |
| **PXFS Server** | server.h, session.h, publisher.h, ... |
| **KVFS Client** | client.h, table.h, sb.h, ... |
| **KVFS Server** | server.h, session.h, subtable.h, publisher.h, ... |
| **CFS Client** | client.h, file.h, ... |
| **CFS Server** | server.h, session.h, namespace.h, ... |

---

## CMake Integration (After Refactoring)

### How to Use Libraries in Your Project

#### PXFS Client
```cmake
target_link_libraries(my_app PRIVATE pxfsc)
target_include_directories(my_app PRIVATE src/pxfs/client)

# Usage:
#   #include "c_api.h"
#   libfs_init2("server_host");
```

#### PXFS Server
```cmake
target_link_libraries(my_server PRIVATE pxfss)
target_include_directories(my_server PRIVATE src/pxfs/server)
```

#### KVFS Client
```cmake
target_link_libraries(my_app PRIVATE kvfsc)
target_include_directories(my_app PRIVATE src/kvfs/client)

# Usage:
#   #include "c_api.h"
#   kvfs_put(key, value, size);
#   kvfs_get(key, buffer);
```

#### KVFS Server
```cmake
target_link_libraries(my_server PRIVATE kvfss)
target_include_directories(my_server PRIVATE src/kvfs/server)
```

### Explicit Public/Private Dependencies

The CMakeLists files now clearly separate:

```cmake
# Public interface (what applications see)
target_link_libraries(pxfsc PUBLIC aerie_includes)

# Private implementation details (hidden from users)
target_link_libraries(pxfsc PRIVATE mfsclt osdclt bcsclt scm common)
```

**Benefits:**
- CMake enforces interface contracts
- Prevents accidental dependency on implementation details
- Easier to refactor internals in the future
- Clear separation of concerns

---

## Improvements Made

### ✅ Naming Clarity
- Renamed `fsc` → `pxfsc`, `fss` → `pxfss`
- Consistent with all other filesystem libraries (kvfsc, cfsc, etc.)

### ✅ API Organization
- Documented public vs private headers for each filesystem
- Clear include directory structure
- Explicit PUBLIC/PRIVATE dependencies in CMakeLists

### ✅ Documentation
- Each library has clear comments about public API
- Public headers are documented with usage examples
- Private headers are marked as internal

### ✅ CMake Structure
- Better layering (metadata → filesystem → executable)
- Clear section headers for each library layer
- Explicit dependency separation (PUBLIC vs PRIVATE)

---

## Migration Guide (If Upgrading Existing Code)

### If You Were Using fsc/fss

**Old:**
```cmake
target_link_libraries(my_app PRIVATE fsc)
```

**New:**
```cmake
target_link_libraries(my_app PRIVATE pxfsc)
```

**In your code:**
```cpp
// Headers don't change
#include "c_api.h"
#include "client.h"

// Usage doesn't change
libfs_init2("server");
libfs_open("/file", O_RDONLY);
```

**Only CMakeLists.txt and build scripts need updates.**

---

## Production Readiness

| Aspect | Status | Details |
|--------|--------|---------|
| **Naming consistency** | ✅ Done | pxfsc/pxfss naming scheme |
| **API organization** | ✅ Done | Clear public/private separation |
| **Documentation** | ✅ Done | Documented in CMakeLists |
| **Interface contracts** | ✅ Done | CMake PUBLIC/PRIVATE distinctions |
| **Include structure** | ✅ Done | Proper include paths configured |
| **Backward compatibility** | ⚠️ Breaking | fsc/fss → pxfsc/pxfss (requires CMakeLists update) |

---

## Next Steps (Optional Improvements)

### Option 1: Create Public Header Wrappers
Copy public headers to a unified `include/` directory:
```
include/
  pxfs/
    c_api.h (copy of src/pxfs/client/c_api.h)
    fs.h    (copy of src/pxfs/server/fs.h)
  kvfs/
    c_api.h
    fs.h
  cfs/
    c_api.h
    fs.h
```

**Benefit:** Clean separation of public vs implementation directories
**Effort:** Medium (copy + update include paths)

### Option 2: Install Targets
Add CMake install rules:
```cmake
install(FILES client/c_api.h DESTINATION include/pxfs)
install(FILES server/fs.h DESTINATION include/pxfs)
install(TARGETS pxfsc DESTINATION lib)
```

**Benefit:** Support proper library installation
**Effort:** Low (standard CMake pattern)

### Option 3: Version Numbering
Add library version information:
```cmake
set_target_properties(pxfsc PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
)
```

**Benefit:** Better library management
**Effort:** Low

---

## Summary of Changes

| File | Change | Impact |
|------|--------|--------|
| `src/pxfs/CMakeLists.txt` | Renamed libraries, improved organization | High |
| `src/kvfs/CMakeLists.txt` | Improved organization, added documentation | Medium |
| `src/cfs/CMakeLists.txt` | Improved organization, added documentation | Medium |
| `bench/ubench/CMakeLists.txt` | Updated pxfsc/pxfss references | High |

**Breaking Change:** Code using `fsc`/`fss` must update to `pxfsc`/`pxfss`

---

**Refactoring Completed**: 2026-05-30
**Status**: ✅ Phase 1 & 2 Complete
**Next Step**: Optional Phase 3 (install targets, version numbering)
