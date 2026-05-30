# Aerie Library Structure Analysis & Recommendations

## Current Library Organization

### Already Implemented ✅

The codebase **already has a good modular structure** with separate client/server libraries for each filesystem:

#### PXFS (POSIX Filesystem)
```
Layer 1 (Backend):
  mfsclt (metadata filesystem client)   → shared library
  mfssrv (metadata filesystem server)   → shared library

Layer 2 (FS):
  fsc    (pxfs client library)          → shared library
  fss    (pxfs server library)          → shared library

Layer 3 (Executables):
  pxfs_server  → uses: fss, bcssrv, osdsrv, scm, common
  pxfs_client  → uses: fsc, mfsclt, bcsclt, osdclt, scm, common
  pxfs_mkfs    → uses: fss, bcssrv, osdsrv, scm, common
```

#### KVFS (Key-Value Filesystem)
```
Layer 1 (FS):
  kvfsc (kvfs client library)           → shared library
  kvfss (kvfs server library)           → shared library

Layer 2 (Executables):
  kvfs_server  → uses: kvfss, bcssrv, osdsrv, scm, common
  kvfs_mkfs    → uses: kvfss, bcssrv, osdsrv, scm, common
```

#### CFS (Cache Filesystem)
```
Layer 1 (FS):
  cfsc (cfs client library)             → shared library
  cfss (cfs server library)             → shared library

Layer 2 (Executables):
  cfs_server   → uses: cfss, bcssrv, osdsrv, scm, common
  cfs_mkfs     → uses: cfss, bcssrv, osdsrv, scm, common
```

#### RXFS (Read-Only PXFS Client)
```
Layer 1 (FS):
  rxfsc (rxfs client library)           → shared library

Layer 2 (Executables):
  ubench_rxfs  → uses: rxfsc, bcsclt, osdclt, scm, common
```

---

## Shared Infrastructure Libraries

All filesystems depend on these lower-level libraries:

```
scm      → Storage Class Memory abstraction
common   → Utility functions, logging, etc.
osd      → Object Storage Device (OSD) layer
         ├── osdclt (OSD client library)
         └── osdsrv (OSD server library)
bcs      → Backend Communication System
         ├── bcsclt (BCS client library)
         └── bcssrv (BCS server library)
```

---

## Current Dependencies Graph

```
                        Applications
                     /      |      \
                  pxfs    kvfs    cfs
                   |       |       |
            ┌──────┴──┐  ┌─┴──┐  ┌─┴──┐
            |         |  |    |  |    |
           fsc       fss kvfsc kvfss cfsc cfss
            |         |  |    |  |    |
        mfsclt    mfssrv |    |  |    |
            \       /     |    |  |    |
             \     /      |    |  |    |
         ┌────────────────┴────┴──┴────┘
         |
      ┌──┴──────────────────────────────┐
      |                                  |
    osdclt/osdsrv              bcsclt/bcssrv
      |                                  |
      └──────────────┬───────────────────┘
                     |
              ┌──────┴──────┐
              |             |
             scm         common
```

---

## Naming Consistency Issue

**Current naming is slightly inconsistent:**
- `fsc` / `fss` ← Could be clearer (what is "fs"?)
- `mfsclt` / `mfssrv` ← Clear (metadata filesystem)
- `kvfsc` / `kvfss` ← Clear (key-value filesystem)
- `cfsc` / `cfss` ← Clear (cache filesystem)
- `rxfsc` ← Clear (read-only client)

---

## Recommended Improvements

### 1. Rename PXFS Libraries for Clarity ✅ RECOMMENDED

**Current:**
```cmake
fsc  → pxfs client library
fss  → pxfs server library
```

**Proposed:**
```cmake
pxfsc  → pxfs client library
pxfss  → pxfs server library
```

**Benefits:**
- Consistent with kvfsc/kvfss, cfsc/cfss naming
- Immediately clear which filesystem it belongs to
- Easier to grep for all pxfs-related code

**Impact:**
- Must update: CMakeLists.txt (pxfs, bench, any code that links fsc/fss)
- Must update: Any documentation/tutorials
- Must update: CI/CD if it references these libraries

### 2. Create Public Header Organization ✅ RECOMMENDED

Create dedicated public header directories:

```
src/
  pxfs/
    include/
      pxfs/
        c_api.h          (public API)
        types.h
    client/
      *.cc  (implementation, private)
    server/
      *.cc  (implementation, private)
    CMakeLists.txt
      target_include_directories(pxfsc PUBLIC include)
      target_include_directories(pxfss PUBLIC include)
```

**Benefits:**
- Clear separation of public vs private headers
- Users know what's stable API vs internal details
- Better for external integration

### 3. Add Explicit PUBLIC Dependencies ✅ RECOMMENDED

**Current:**
```cmake
target_link_libraries(fsc PUBLIC aerie_includes mfsclt osdclt bcsclt scm common)
```

Should document which are public API dependencies:

```cmake
# Public API (users link against these)
target_link_libraries(pxfsc PUBLIC aerie_includes)

# Implementation details (private to pxfs)
target_link_libraries(pxfsc PRIVATE mfsclt osdclt bcsclt scm common)
```

**Benefits:**
- CMake better enforces interface contracts
- Prevents accidental dependency on internals
- Easier to refactor internals later

### 4. Create Convenience Wrapper Libraries ⚠️ OPTIONAL

For applications that need both client and server:

```cmake
add_library(pxfs_all_libs INTERFACE)
target_link_libraries(pxfs_all_libs INTERFACE pxfsc pxfss bcsclt bcssrv osdclt osdsrv)
```

**Usage:**
```cmake
target_link_libraries(my_app PRIVATE pxfs_all_libs)
```

---

## Summary of Changes

### Phase 1: Naming Consistency (Low Risk)
1. Rename `fsc` → `pxfsc` in pxfs/CMakeLists.txt
2. Rename `fss` → `pxfss` in pxfs/CMakeLists.txt
3. Update bench/ubench/CMakeLists.txt to use `pxfsc`, `pxfss`
4. Update any linking in build scripts

**Time**: ~30 minutes
**Risk**: Low - mostly mechanical renames
**Impact**: Better code clarity

### Phase 2: Public Headers (Medium Effort)
1. Create `src/pxfs/include/pxfs/` directory
2. Move public headers: c_api.h, types.h, etc.
3. Update CMakeLists to expose include directory
4. Update internal includes as needed

**Time**: ~1-2 hours
**Risk**: Medium - needs testing to ensure no includes break
**Impact**: Better API clarity

### Phase 3: Interface Improvements (Optional)
1. Add explicit PRIVATE vs PUBLIC in CMakeLists
2. Create convenience wrapper libraries if needed
3. Document public vs private APIs

**Time**: ~1 hour
**Risk**: Low - documentation/CMake improvements
**Impact**: Easier maintenance and future refactoring

---

## Current State Assessment

**Strengths** ✅
- Already have separate client/server libraries per filesystem
- Good layered architecture (metadata → filesystem → executable)
- Clean separation of OSD, BCS, SCM layers
- Shared infrastructure well-isolated

**Weaknesses** ⚠️
- PXFS naming (`fsc`/`fss`) less clear than others
- No explicit public header directory structure
- No clear public vs private API distinction
- Could benefit from convenience wrapper libraries

**Overall Grade**: **B+** (Good structure, minor improvements possible)

---

## Recommendation for Next Steps

**If you want to improve library organization:**

1. **Quick win** (30 min): Rename `fsc` → `pxfsc`, `fss` → `pxfss`
2. **Medium effort** (1-2 hr): Add public header directories
3. **Optional** (1 hr): Add explicit PUBLIC/PRIVATE distinctions

**If current structure works well:**
- No changes needed, libraries are already well-organized
- Can continue with testing and other improvements

---

## Quick Command to List All Libraries

```bash
find src -name CMakeLists.txt -exec grep "add_library(" {} + | sort
```

**Output:**
```
mfsclt   - PXFS metadata filesystem client
mfssrv   - PXFS metadata filesystem server
fsc      - PXFS client library
fss      - PXFS server library
kvfsc    - KVFS client library
kvfss    - KVFS server library
cfsc     - CFS client library
cfss     - CFS server library
rxfsc    - RXFS (read-only) client library
osdclt   - OSD client library
osdsrv   - OSD server library
bcsclt   - BCS client library
bcssrv   - BCS server library
scm      - Storage Class Memory
common   - Common utilities
```

---

**Document Created**: 2026-05-30
**Status**: Analysis complete, recommendations provided
**Next Action**: User decides if improvements are desired
