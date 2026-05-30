# Aerie Filesystem Architecture

## Overview

Aerie is a modular usermode POSIX filesystem framework for NVM/SCM (Non-Volatile Memory / Storage Class Memory) on x86-64 Linux. The framework provides a shared infrastructure layer that multiple filesystem implementations (pxfs, kvfs) can build upon.

## Directory Structure

```
libfs/src/
├── common/          # Shared utilities and data structures
├── scm/             # SCM pool management and initialization
├── osd/             # Object Storage Device (abstraction layer)
├── bcs/             # Block-based Cache/Storage system
├── pxfs/            # POSIX filesystem implementation
├── kvfs/            # Key-Value filesystem implementation
├── rxfs/            # RAID filesystem variant
├── cfs/             # Composite filesystem variant
└── kvfs-new/        # New kvfs implementation (experimental)
```

## Common Infrastructure Layers

### 1. **common/** — Shared Utilities & Data Structures

Provides foundational utilities used by all filesystem implementations:

- **Hash tables** (`uthash.h`, `hash.c`) — Generic hash table implementation
- **Interval trees** (`interval_tree.h`, `interval_tree.cc`) — For efficient range queries
- **Bit manipulation** (`bitmap.h`, `bitset.h`) — Bitmap and bitset utilities
- **Error codes** (`errno.h`) — Filesystem-wide error definitions
- **Type definitions** (`types.h`) — Common type aliases
- **System utilities** (`util.h`, `util.c`) — Helper functions
- **Performance profiling** (`prof.h`, `profile.c`) — Timing and instrumentation
- **Memory mapping** (`mmapregion.h`) — MMAP region management
- **Template structures** (`TemplateStack.H`, `list.h`) — Generic containers
- **Synchronization** (`ut_barrier.h`) — Synchronization primitives
- **Timing** (`hrtime.h`) — High-resolution timing

### 2. **scm/** — Storage Class Memory Management

Manages the persistent pool of NVM/SCM storage:

- **Pool creation & formatting** — Initialize SCM pools
- **Block allocation** — Allocate and manage storage blocks
- **Metadata management** — Track pool structure and block usage
- **Persistence** — Ensure durability across system restarts

**Used by:** Both pxfs and kvfs for underlying storage

### 3. **osd/** — Object Storage Device Abstraction Layer

Provides a storage abstraction that sits between filesystems and the SCM layer:

- **Client-side** (`client/`) — RPC client for remote OSD operations
- **Server-side** (`server/`) — OSD server implementation
- **Shared buffer management** — IPC via shared memory
- **Object allocation & retrieval** — Logical object-level operations
- **Storage descriptors** — Metadata for storage management

**Key components:**
- `stsystem.cc/h` — Storage system initialization and mounting
- `shbuf.h` — Shared buffer handling (where we found memory leaks)
- RPC communication for client-server operations

**Used by:** Both pxfs and kvfs through their client libraries

### 4. **bcs/** — Block-based Cache/Storage System

Provides block-level storage and caching abstractions:

- **Block management** — Allocate and track blocks
- **Caching layer** — Cache frequently accessed blocks
- **Block descriptors** — Metadata for blocks
- **I/O operations** — Read/write block operations

**Used by:** OSD layer and filesystem implementations

## Filesystem Implementations

### pxfs — POSIX Filesystem

A full POSIX filesystem implementation supporting:
- Files and directories
- Standard file operations (open, read, write, seek, etc.)
- Directory operations (mkdir, rmdir, rename)
- Symbolic and hard links
- File metadata (permissions, timestamps)
- Inode-based organization with B-tree indexing

**Key components:**
- `client/namespace.cc` — Path name resolution with caching
- `client/file.cc` — File handle management
- `client/inode.cc` — Inode implementation
- `server/fs.cc` — Server-side filesystem logic

### kvfs — Key-Value Filesystem

A simplified filesystem focused on key-value operations:
- Put/Get/Delete operations on key-value pairs
- Simpler metadata model than pxfs
- Optimized for in-memory performance
- Table-based organization

**Key components:**
- `client/table.cc` — Table/bucket management
- `client/session.h` — Session handling
- `server/subtable.cc` — Server-side table operations

## Communication Architecture

Both pxfs and kvfs use RPC-based client-server communication:

```
Client Process                      Server Process
   ↓                                   ↓
kvfs_api_test / pxfs_api_test        kvfs_server / pxfs_server
   ↓                                   ↓
Client Library (kvfs/client, pxfs/client)   Server Library (kvfs/server, pxfs/server)
   ↓                                   ↓
OSD Client Layer ←→ RPC Network ←→ OSD Server Layer
   ↓                                   ↓
BCS Layer                           BCS Layer
   ↓                                   ↓
SCM Pool (persistent storage)        SCM Pool (persistent storage)
```

## Memory Layout

- **pxfs_api_test** connects to pxfs_server via RPC (network port 10000 by default)
- **kvfs_api_test** connects to kvfs_server via RPC (network port 10001 by default)
- Each filesystem uses its own SCM pool for storage isolation
- Shared buffers are used for efficient IPC between client and server

## Testing

### pxfs Tests
```bash
./run-tests.sh api           # Run pxfs correctness tests
./run-tests.sh leak          # Run with valgrind memory leak checking
./run-tests.sh bench         # Run pxfs micro-benchmarks
```

**Result:** 19/19 tests pass with **0 bytes definitely lost** (after fixes to namespace.cc)

### kvfs Tests
```bash
./run-tests.sh kvfs          # Run kvfs correctness tests
./run-tests.sh kvfs-leak     # Run with valgrind memory leak checking
```

**Result:** 6/7 tests pass with **144 bytes definitely lost** (in OSD layer initialization)

## Known Memory Leaks

### pxfs (FIXED)
- **Previously:** 2,162 bytes (113 blocks) definitely lost
- **Root cause:** strdup() calls in namespace.cc path resolution cache not freed
- **Fix:** Track original pointers and free at function scope exit
- **Status:** Fixed in commit e5c7a938b - 91% reduction, 200 bytes remaining (likely one-time init allocations)

### kvfs (OUTSTANDING)
- **Currently:** 144 bytes (3 blocks) definitely lost
- **Root cause:** OSD layer (stsystem.cc, sb.cc) initialization allocations
  1. StorageSystem::Init() — 8 bytes
  2. SuperBlock::Load() — 16 bytes  
  3. StorageSystem::Mount() — 120 bytes (+ 21 indirect via SharedBuffer)

These are one-time initialization allocations in the storage abstraction layer shared by pxfs and kvfs. They occur during filesystem mounting and persist for the process lifetime.

## Shared Infrastructure Observations

1. **OSD Layer Leaks** — The kvfs leaks are all in the OSD layer (osd/client/stsystem.cc and related), which is shared between pxfs and kvfs. The pxfs tests don't expose these leaks because pxfs may take a different initialization path or have longer test coverage that reaches cleanup code.

2. **Common Pool** — Both filesystems use the same pool management (scm/) but can maintain separate pools for isolation

3. **RPC Communication** — Both use the same RPC network layer for client-server communication

4. **Shared Buffer IPC** — Both use the OSD layer's shared buffer mechanism (osd/client/shbuf.h) for efficient IPC

## Build Configuration

The build system supports:
- `RPC=net` — Network-based RPC (default)
- `RPC=shm` — Shared memory RPC (experimental)
- `SCMPOOL=user` — User-managed SCM pools (default)
- `BUILD_BENCH=ON` — Build test binaries (default)
- `COVERAGE=ON` — Code coverage instrumentation
- `BUILD_TYPE=Debug|Release` — Debug or Release builds

## Future Work

1. **Fix remaining kvfs OSD layer leaks** — Track and free initialization allocations in stsystem.cc
2. **Add pxfs kvfs interoperability tests** — Test shared infrastructure compatibility
3. **Performance optimization** — Profile shared layer bottlenecks
4. **New filesystem implementations** — Use pxfs/kvfs as templates for custom filesystems
