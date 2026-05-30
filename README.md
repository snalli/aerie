# Aerie

A usermode filesystem for Non-Volatile Memory (NVM / Storage Class Memory).

Aerie runs entirely in userspace — no kernel filesystem module required. It maps
a persistent storage pool directly into process address space via `mmap` and
provides POSIX filesystem semantics through a client library that communicates
with a local server process over RPC.

## Architecture

```
┌─────────────────────────────────────────┐
│  Application                            │
│  (links libfsc / libkvfsc / libc_api)   │
└────────────────┬────────────────────────┘
                 │ TCP RPC (rpcnet) or shared-memory RPC (rpcfast)
┌────────────────▼────────────────────────┐
│  pxfs_server  /  cfs_server  / kvfs_server │
│  ├── OSD layer  (locking, journaling)   │
│  ├── MFS backend (directory/file inodes)│
│  └── SCM pool   (mmap'd persistent file)│
└─────────────────────────────────────────┘
```

The storage pool is a regular file memory-mapped into the process address space.
No kernel module or special hardware is required; SCM write latency can be
emulated in userspace via `movnti` + busy-spin.

## Filesystem variants

| Name | Server | Client API | Description |
|------|--------|-----------|-------------|
| **pxfs** | `pxfs_server` | `libfs_*` | Full POSIX-compatible filesystem — primary variant |
| **rxfs** | `pxfs_server` | `rxfs_*` | Read-only replica client of pxfs |
| **cfs**  | `cfs_server`  | `cfs_*`  | Cache filesystem — aggressive client-side caching |
| **kvfs** | `kvfs_server` | `kvfs_*` | Key-value store (`put`/`get`/`del`) on the same OSD stack |

### OSD layer

All filesystem variants are built on the **Object Storage Device (OSD)** layer
which provides: distributed lock management (hierarchical + flat), journaling,
versioned object management, and extent/container allocation.  `ubench_osd`
benchmarks the raw OSD layer without any filesystem on top.

## pxfs API (`libfs_*`)

```c
/* lifecycle */
int libfs_init3(const char* xdst, int debug_level);
int libfs_shutdown();
int libfs_mount(const char* source, const char* target, const char* fstype, uint32_t flags);

/* files */
int     libfs_open(const char* path, int flags);
int     libfs_open2(const char* path, int flags, mode_t mode);
int     libfs_close(int fd);
ssize_t libfs_read(int fd, void* buf, size_t n);
ssize_t libfs_write(int fd, const void* buf, size_t n);
ssize_t libfs_pread(int fd, void* buf, size_t n, off_t offset);
ssize_t libfs_pwrite(int fd, const void* buf, size_t n, off_t offset);
off_t   libfs_lseek(int fd, off_t offset, int whence);
int     libfs_fsync(int fd);
int     libfs_sync();
int     libfs_stat(const char* path, struct stat* buf);

/* directories */
int   libfs_mkdir(const char* path, int mode);
int   libfs_rmdir(const char* path);
int   libfs_chdir(const char* path);
char* libfs_getcwd(char* buf, size_t size);

/* namespace */
int libfs_link(const char* oldpath, const char* newpath);
int libfs_unlink(const char* pathname);
int libfs_rename(const char* oldpath, const char* newpath);

/* fd management */
int libfs_dup(int oldfd);
int libfs_dup2(int oldfd, int newfd);
```

## kvfs API (`kvfs_*`)

```c
int     kvfs_init2(const char* xdst);
int     kvfs_shutdown();
int     kvfs_mount(const char* source, uint32_t flags);
ssize_t kvfs_put(const char* key, const void* buf, size_t count);
ssize_t kvfs_get(const char* key, void* buf);
int     kvfs_del(const char* key);
int     kvfs_sync();
```

## Building

### Requirements

- Linux x86-64 (uses `rdtsc`, SSE `movnti`, `cpu_set_t`)
- CMake ≥ 3.16
- GCC with C++11 support
- `libconfig++` (`libconfig++-dev` on Ubuntu)
- Boost headers (`libboost-dev`)
- Google Sparsehash (`libsparsehash-dev`)

### Quick build

```bash
cmake -S libfs -B libfs/build -DRPC=net -DSCMPOOL=user
cmake --build libfs/build --parallel $(nproc)
```

**Build options:**

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `RPC` | `net`, `fast`, `fast-two` | `net` | RPC transport (`net` = TCP; `fast` = shared-memory) |
| `SCMPOOL` | `kernel`, `user` | `kernel` | Pool allocator (`user` works without patched kernel) |
| `BUILD_BENCH` | `ON`, `OFF` | `ON` | Build benchmark and test programs |

> **NVM kernel note:** `SCMPOOL=kernel` uses custom Linux syscalls (312/313)
> for NVM memory protection and is only available on the patched Aerie kernel.
> Use `SCMPOOL=user` for standard Linux.

### Local Docker build (recommended for development)

Reproduces the exact CI environment without installing anything:

```bash
./build-local.sh                           # build only
./build-local.sh 2>&1 | grep -E "warning:|error:"  # check for warnings
```

The script uses `ubuntu:22.04` on `linux/amd64` with the same flags as CI and
caches the build directory in a Docker volume for fast incremental rebuilds.

### Docker Compose (server + bench)

```bash
# Build image and start pxfs server
docker compose up --build

# Run micro-benchmark against the running server
docker compose --profile bench run --rm bench
```

## Running manually

```bash
BUILD=libfs/build

# 1. Create storage pool (user-space allocator, no patched kernel needed)
$BUILD/src/scm/pool_tool create -p /tmp/stamnos_pool -s 128M

# 2. Format the filesystem
$BUILD/src/pxfs/pxfs_mkfs create -p /tmp/stamnos_pool -s 128M -t mfs

# 3. Start the server
export LIBFS_CONFIG=$PWD/libfs/libfs.ini
$BUILD/src/pxfs/pxfs_server -p 10000 -d 0 -s /tmp/stamnos_pool &

# 4. Run API unit tests
$BUILD/bench/ubench/pxfs_api_test -h 10000

# 5. Run micro-benchmarks (all ops in one process invocation)
$BUILD/bench/ubench/ubench_pxfs -h 10000 \
  +fs_create -n 100 -p /pxfs \
  +fs_open   -n 100 -p /pxfs \
  +fs_read   -n 100 -p /pxfs
```

Configuration is read from `libfs/libfs.ini` (override with `$LIBFS_CONFIG`).

## Benchmarks & tests

### Micro-benchmarks (`ubench`)

Each binary runs identical operations against its filesystem backend:

| Binary | Filesystem | Server needed |
|--------|-----------|--------------|
| `ubench_pxfs` | pxfs | `pxfs_server` |
| `ubench_rxfs` | rxfs (read-only) | `pxfs_server` |
| `ubench_cfs`  | cfs  | `cfs_server` |
| `ubench_vfs`  | Linux VFS (baseline) | none |
| `ubench_osd`  | OSD layer | `pxfs_server` |

Available operations: `+fs_create`, `+fs_open`, `+fs_read`, `+fs_seqwrite`,
`+fs_seqread`, `+fs_randread`, `+fs_randwrite`, `+fs_append`, `+fs_rename`,
`+fs_unlink`, `+fs_delete`, `+fs_fread`

Options: `-n <ops>` `-p <root_path>` `-s <size>`

### API correctness tests

```bash
# pxfs — tests open/close/read/write/pread/pwrite/lseek/mkdir/rmdir/
#         rename/link/unlink/stat/dup/chdir/sync/fsync
pxfs_api_test -h 10000

# kvfs — tests put/get/del/overwrite/binary values/large values/sync
kvfs_api_test -h 10000
```

## CI

Every push runs three jobs:

| Job | What it does |
|-----|-------------|
| **CMake build** | Release build with `-Wall -Wextra` (must be warning-free) |
| **ubench** | Smoke-tests pxfs/rxfs/osd/cfs/vfs with 1 op each; results in step summary |
| **Docker** | Builds the runtime image; pushes to GHCR on version tags |

## Repository layout

```
libfs/
  src/
    bcs/       Basic Communication Services (TCP/shared-mem RPC + IPC)
    scm/       SCM abstraction (persistent regions, pool allocator)
    osd/       Object Storage Device layer (locking, journaling, versioning)
    pxfs/      POSIX filesystem (server + client + mkfs)
    cfs/       Cache filesystem variant
    rxfs/       Read-only client variant
    kvfs/      Key-value store variant
    common/    Shared utilities, ASSERT_OK macro
  bench/
    ubench/    Micro-benchmarks + API unit tests (pxfs_api_test, kvfs_api_test)
  scripts/     Docker entrypoint scripts
  libfs.ini    Runtime configuration (debug levels, module flags)
  build-local.sh  Reproduce CI build locally via Docker
kernelmode/
  scmdisk/     Optional kernel block device emulating SCM latency
```

## Dependencies

- [libconfig++](http://hyperrealm.github.io/libconfig/)
- [Boost C++ Libraries](https://www.boost.org/)
- [Google Sparsehash](https://github.com/sparsehash/sparsehash)
