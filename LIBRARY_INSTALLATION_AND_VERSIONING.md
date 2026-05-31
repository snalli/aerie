# Library Installation and Versioning Guide

## Overview

This document describes the installation and versioning configuration for Aerie's filesystem libraries.

---

## Version Information

All filesystem libraries now include semantic versioning:

```cmake
set_target_properties(pxfsc PROPERTIES
    VERSION 1.0.0      # Library version
    SOVERSION 1        # ABI compatibility version
)
```

### Version Numbering Scheme

**Format:** `MAJOR.MINOR.PATCH`

- **MAJOR (1)**: ABI-breaking changes
- **MINOR (0)**: New features, backward compatible
- **PATCH (0)**: Bug fixes only

### Current Versions

| Library | Version | SOVERSION | Notes |
|---------|---------|-----------|-------|
| **pxfsc** | 1.0.0 | 1 | PXFS client library |
| **pxfss** | 1.0.0 | 1 | PXFS server library |
| **kvfsc** | 1.0.0 | 1 | KVFS client library |
| **kvfss** | 1.0.0 | 1 | KVFS server library |
| **cfsc** | 1.0.0 | 1 | CFS client library |
| **cfss** | 1.0.0 | 1 | CFS server library |

---

## Symbol Visibility

All libraries are compiled with hidden symbol visibility to prevent ABI leaks:

```cmake
set_target_properties(pxfsc PROPERTIES
    COMPILE_FLAGS "-fvisibility=hidden"
)
```

### Benefits of Hidden Symbols

- ✅ **Smaller libraries**: Unused symbols not exposed
- ✅ **Faster loading**: Linker has less work to do
- ✅ **Better ABI stability**: Only intentional APIs are exposed
- ✅ **Fewer symbol conflicts**: No accidental global symbols
- ✅ **Security**: Internal implementation details hidden

### How It Works

1. Symbols are hidden by default: `-fvisibility=hidden`
2. Only explicitly marked symbols are visible
3. Public API functions remain accessible
4. Implementation details are completely hidden

---

## Installation Configuration

### Install Targets

Libraries are installed to standard locations:

```cmake
install(TARGETS pxfsc
    LIBRARY DESTINATION lib        # Shared libraries (.so)
    ARCHIVE DESTINATION lib        # Static libraries (.a)
)
```

### Install Paths

**After installation, libraries are located at:**

```
/usr/local/lib/
├── libpxfsc.so.1.0.0   (PXFS client with version)
├── libpxfsc.so.1       (ABI version symlink)
├── libpxfsc.so         (Latest version symlink)
├── libpxfss.so.1.0.0   (PXFS server with version)
├── libpxfss.so.1
├── libpxfss.so
├── libkvfsc.so.1.0.0   (KVFS client with version)
├── libkvfsc.so.1
├── libkvfsc.so
├── libkvfss.so.1.0.0   (KVFS server with version)
├── libkvfss.so.1
├── libkvfss.so
├── libcfsc.so.1.0.0    (CFS client with version)
├── libcfsc.so.1
├── libcfsc.so
├── libcfss.so.1.0.0    (CFS server with version)
├── libcfss.so.1
└── libcfss.so

/usr/local/include/
├── pxfs/
│   ├── c_api.h
│   └── fs.h
├── kvfs/
│   ├── c_api.h
│   └── fs.h
└── cfs/
    ├── c_api.h
    └── fs.h
```

### Install Headers

Public headers are installed to include directories:

```cmake
install(FILES ${CMAKE_SOURCE_DIR}/include/pxfs/c_api.h DESTINATION include/pxfs)
install(FILES ${CMAKE_SOURCE_DIR}/include/pxfs/fs.h DESTINATION include/pxfs)
```

---

## How to Install Libraries

### Build and Install

```bash
cd libfs
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build
```

### Verify Installation

```bash
# Check installed libraries
ls -la /usr/local/lib/libpxfs*.so*
ls -la /usr/local/lib/libkvfs*.so*
ls -la /usr/local/lib/libcfs*.so*

# Check installed headers
ls -la /usr/local/include/pxfs/
ls -la /usr/local/include/kvfs/
ls -la /usr/local/include/cfs/

# Test linking
gcc -I/usr/local/include -L/usr/local/lib \
    -o test_pxfs test.c -lpxfsc -lm
```

---

## Using Installed Libraries

### Linking Against Installed Libraries

```cmake
# In your CMakeLists.txt
find_library(PXFSC_LIB pxfsc PATHS /usr/local/lib)
find_path(PXFS_INCLUDE pxfs/c_api.h PATHS /usr/local/include)

target_link_libraries(my_app PRIVATE ${PXFSC_LIB})
target_include_directories(my_app PRIVATE ${PXFS_INCLUDE})
```

### Or with pkg-config (if configured)

```bash
gcc -I$(pkg-config --cflags aerie-pxfs) \
    -o test_pxfs test.c \
    $(pkg-config --libs aerie-pxfs)
```

---

## Symbol Visibility in Practice

### Public Symbols (Visible)

All public API functions:
- `libfs_init()`
- `libfs_open()`
- `libfs_read()`
- `kvfs_put()`
- `kvfs_get()`
- etc.

### Private Symbols (Hidden)

Implementation details:
- `Client::Client()` (C++ constructor)
- `Table::insert()` (internal function)
- `Session::handle_request()` (internal function)
- etc.

**Benefit:** Users can only call public API, cannot accidentally use internals.

---

## Version Compatibility

### Backward Compatibility

**SOVERSION 1** means:

- ✅ All 1.0.0, 1.1.0, 1.2.0 are compatible with 1.0.0
- ❌ 2.0.0 is NOT compatible (ABI break)

### Using Correct SOVERSION

```cmake
# Same SOVERSION = backward compatible
libpxfsc.so.1.0.0  (version 1.0.0)
libpxfsc.so.1.1.0  (version 1.1.0)  ← Still SOVERSION 1
libpxfsc.so.1      → libpxfsc.so.1.1.0  (points to latest 1.x)

# Different SOVERSION = breaking change
libpxfsc.so.2.0.0  (version 2.0.0)   ← New SOVERSION 2
libpxfsc.so.2      → libpxfsc.so.2.0.0  (can't use old 1.x)
```

---

## Installation Directory Structure (After Install)

```
/usr/local/
├── lib/
│   ├── libpxfsc.so → libpxfsc.so.1
│   ├── libpxfsc.so.1 → libpxfsc.so.1.0.0
│   ├── libpxfsc.so.1.0.0 (actual file)
│   ├── libpxfss.so → libpxfss.so.1
│   ├── libpxfss.so.1 → libpxfss.so.1.0.0
│   ├── libpxfss.so.1.0.0 (actual file)
│   ├── (similar for kvfs and cfs)
│   └── ...
│
├── include/
│   ├── pxfs/
│   │   ├── c_api.h (public)
│   │   └── fs.h (public)
│   ├── kvfs/
│   │   ├── c_api.h (public)
│   │   └── fs.h (public)
│   ├── cfs/
│   │   ├── c_api.h (public)
│   │   └── fs.h (public)
│   └── ...
│
└── share/
    ├── doc/aerie/ (optional)
    └── ...
```

---

## Environment Setup

### For Development (Before Install)

```bash
export LD_LIBRARY_PATH=/path/to/aerie/libfs/build/src/pxfs:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/aerie/libfs/build/src/kvfs:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/aerie/libfs/build/src/cfs:$LD_LIBRARY_PATH

# Or in CMakeLists.txt:
target_link_directories(my_app PRIVATE ${CMAKE_BINARY_DIR}/src/pxfs)
```

### For Production (After Install)

```bash
# Libraries are in /usr/local/lib, which is in default search path
# No special environment setup needed
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH
```

---

## Updates and Future Versions

### Version Bumping Strategy

**For new releases:**

1. **1.0.0 → 1.1.0**: New features, backward compatible
   - Keep SOVERSION = 1
   - New public API functions allowed
   - Existing API unchanged

2. **1.1.0 → 1.2.0**: Bug fixes
   - Keep SOVERSION = 1
   - No API changes
   - Internal improvements only

3. **1.x.x → 2.0.0**: Breaking changes
   - Change SOVERSION = 2
   - API changes allowed
   - Requires users to rebuild/relink

### Example Version Bump

```cmake
# In src/pxfs/CMakeLists.txt for version 1.1.0
set_target_properties(pxfsc PROPERTIES
    VERSION 1.1.0      # Changed from 1.0.0
    SOVERSION 1        # Stays the same (compatible)
)

# For version 2.0.0 with breaking changes
set_target_properties(pxfsc PROPERTIES
    VERSION 2.0.0      # Major bump
    SOVERSION 2        # Incompatible - different SOVERSION
)
```

---

## Summary of Changes

| Improvement | Before | After |
|-------------|--------|-------|
| **Versioning** | ❌ None | ✅ 1.0.0 semantic versioning |
| **SOVERSION** | ❌ None | ✅ SOVERSION 1 for ABI tracking |
| **Symbol Visibility** | ❌ All exported | ✅ Hidden by default |
| **Installation** | ❌ Manual copy | ✅ CMake install targets |
| **Install Paths** | ❌ Custom | ✅ Standard lib/ and include/ |
| **Header Installation** | ❌ Not packaged | ✅ Installed with libraries |

---

## Complete CMakeLists Configuration

**Example: PXFS client library with all improvements**

```cmake
add_library(pxfsc SHARED
    client/c_api.cc
    client/client.cc
    client/file.cc
    client/fsomgr.cc
    client/inode.cc
    client/mpinode.cc
    client/namespace.cc
)

# Public interface
target_include_directories(pxfsc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/client>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(pxfsc PUBLIC aerie_includes)

# Private dependencies
target_link_libraries(pxfsc PRIVATE mfsclt osdclt bcsclt scm common)

# ✅ Version information
set_target_properties(pxfsc PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    COMPILE_FLAGS "-fvisibility=hidden"
)

# ✅ Install rules
install(TARGETS pxfsc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(FILES ${CMAKE_SOURCE_DIR}/include/pxfs/c_api.h 
    DESTINATION include/pxfs)
```

---

## Verification Checklist

- [x] All libraries have VERSION set (1.0.0)
- [x] All libraries have SOVERSION set (1)
- [x] All libraries compiled with -fvisibility=hidden
- [x] Install rules added for all libraries
- [x] Public headers installed to include/
- [x] Libraries installed to lib/
- [x] Installation follows standard FHS (Filesystem Hierarchy Standard)

---

**Implementation Completed**: 2026-05-30
**Status**: ✅ All Optional Improvements Complete
**Ready for Distribution**: ✅ Yes
