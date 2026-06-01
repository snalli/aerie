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
│  (links libpxfsc / libkvfsc / libcfsc)  │
└────────────────┬────────────────────────┘
                 │ TCP RPC (rpcnet) or shared-memory RPC (rpcfast)
┌────────────────▼────────────────────────┐
│  pxfs_server / cfs_server / kvfs_server │
│  ├── OSD layer  (locking, journaling)   │
│  ├── MFS backend (directory/file inodes)│
│  └── SCM pool   (mmap'd persistent file)│
└─────────────────────────────────────────┘
```

The storage pool is a regular file memory-mapped into the process address space.
No kernel module or special hardware is required; SCM write latency can be
emulated in userspace via `movnti` + busy-spin.

> **Platform:** x86-64 Linux only — the codebase uses `rdtsc`, SSE `movnti`,
> and x86 inline assembly. It will not compile on ARM or other architectures.

## Filesystem variants

| Name | Server | Client API | Description |
|------|--------|-----------|-------------|
| **pxfs** | `pxfs_server` | `libfs_*` | Full POSIX-compatible filesystem — primary variant |
| **rxfs** | `pxfs_server` | `rxfs_*`  | Read-only replica client of pxfs |
| **cfs**  | `cfs_server`  | `cfs_*`   | Cache filesystem — aggressive client-side caching |
| **kvfs** | `kvfs_server` | `kvfs_*`  | Key-value store (`put`/`get`/`del`) on the same OSD stack |

### OSD layer

All filesystem variants are built on the **Object Storage Device (OSD)** layer
which provides: distributed lock management (hierarchical + flat), journaling,
versioned object management, and extent/container allocation. `ubench_osd`
benchmarks the raw OSD layer without any filesystem on top.

## Libraries built

| Library | Description |
|---------|-------------|
| `librpcnet` / `librpcfast` | RPC transports (TCP / shared memory) |
| `libbcsclt` / `libbcssrv` | Basic Communication Services client/server |
| `libcommon` / `libscm` | Shared utilities and SCM pool abstraction |
| `libosdclt` / `libosdsrv` | OSD client/server |
| `libmfsclt` / `libmfssrv` | Metadata filesystem client/server |
| `libpxfsc` / `libpxfss` | PXFS client/server |
| `libcfsc` / `libcfss` | CFS client/server |
| `librxfsc` | RXFS read-only client |
| `libkvfsc` / `libkvfss` | KVFS client/server |

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

- Linux x86-64
- CMake ≥ 3.16
- GCC with C++11 support
- `libconfig++-dev`
- `libboost-dev`
- `libsparsehash-dev`

### Quick build

```bash
cmake -S libfs -B libfs/build
cmake --build libfs/build --parallel $(nproc)
```

**Build options** (defaults shown):

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `RPC` | `net`, `fast`, `fast-two` | `net` | RPC transport (`net` = TCP; `fast` = shared-memory) |
| `SCMPOOL` | `user`, `kernel` | `user` | Pool allocator (`user` works without patched kernel) |
| `BUILD_BENCH` | `ON`, `OFF` | `ON` | Build benchmark and test programs |

> **`SCMPOOL=kernel`** requires custom Linux syscalls (312/313) for NVM memory
> protection, only available on the patched Aerie kernel. Use `SCMPOOL=user`
> (the default) for standard Linux.

### Local development with Docker (Apple Silicon / any OS)

The Docker image pins to `linux/amd64` (via QEMU on Apple Silicon) and exactly
matches the CI environment (Ubuntu 22.04, same compiler, same deps):

```bash
# Interactive shell
bash scripts/docker-ci-run.sh bash

# Build
bash scripts/docker-ci-run.sh build

# Format check / fix
bash scripts/docker-ci-run.sh format-code
bash scripts/docker-ci-run.sh format-fix

# Full CI pipeline
bash scripts/docker-ci-run.sh ci-full
```

> **Note:** `pxfs_server` and other server processes require ASLR to be
> disabled (`kernel.randomize_va_space=0`) because the SCM pool uses `MAP_FIXED`
> to remap at its original address. Docker Desktop on Mac cannot set this sysctl,
> so server-dependent tests only run reliably in CI on real x86-64 Linux.
> VFS baseline tests run fine locally.

## Running manually

```bash
BUILD=libfs/build
export LIBFS_CONFIG=$PWD/libfs/libfs.ini

# 1. Create storage pool
$BUILD/src/scm/pool_tool create -p /tmp/stamnos_pool -s 128M

# 2. Format the filesystem
$BUILD/src/pxfs/pxfs_mkfs create -p /tmp/stamnos_pool -s 128M -t mfs

# 3. Start the server
$BUILD/src/pxfs/pxfs_server -p 10000 -s /tmp/stamnos_pool &

# 4. Run API unit tests
$BUILD/bench/ubench/pxfs_api_test -h 10000

# 5. Run micro-benchmarks
$BUILD/bench/ubench/ubench_pxfs -h 10000 \
  +fs_create -n 100 -p /pxfs \
  +fs_open   -n 100 -p /pxfs \
  +fs_read   -n 100 -p /pxfs
```

## Command-line flags

### Servers (`pxfs_server`, `kvfs_server`, `cfs_server`)

| Flag | Description | Default |
|------|-------------|---------|
| `-p <port>` | Listen port | `20000 + (pid % 10000)` |
| `-s <path>` | Storage pool path | _(required)_ |
| `-d <level>` | Debug level (overrides `libfs.ini`) | from config |
| `-l` | Enable lossy RPC for fault testing | off |

### Clients / benchmarks

| Flag | Description | Default |
|------|-------------|---------|
| `-h <host:port>` | Server address | `10000` |
| `-d <level>` | Debug level | `0` |
| `-l` | Enable lossy RPC | off |

## Configuration (`libfs/libfs.ini`)

```ini
debug:
{
    level = 0;              # 0 = silent, higher = more verbose
    module: { all = False; /* per-subsystem flags */ };
};

ipc:
{
    sharedbuffer: { size = "1M"; };   # client↔server shared ring buffer
};

scmmodel:
{
    latency = 0;    # emulated SCM write latency in nanoseconds
};
```

**Every config key can be overridden via environment variable** by uppercasing
and replacing `.` with `_`:

```bash
LIBFS_CONFIG=/path/to/libfs.ini      # use a different config file
DEBUG_LEVEL=0                         # debug.level
IPC_SHAREDBUFFER_SIZE=4M             # ipc.sharedbuffer.size
SCMMODEL_LATENCY=100                 # 100 ns write latency emulation
RPC_LOSSY=5                          # % packet drop (fault injection)
```

## Benchmarks & tests

### Micro-benchmarks (`ubench`)

| Binary | Filesystem | Server needed |
|--------|-----------|--------------|
| `ubench_pxfs` | pxfs | `pxfs_server` |
| `ubench_rxfs` | rxfs (read-only) | `pxfs_server` |
| `ubench_cfs`  | cfs  | `cfs_server` |
| `ubench_osd`  | OSD layer | `pxfs_server` |
| `ubench_vfs`  | Linux VFS (baseline) | none |

Available operations: `+fs_create`, `+fs_open`, `+fs_read`, `+fs_seqwrite`,
`+fs_seqread`, `+fs_randread`, `+fs_randwrite`, `+fs_append`, `+fs_rename`,
`+fs_unlink`, `+fs_delete`, `+fs_fread`

Options per operation: `-n <count>` `-p <root_path>` `-s <size>`

### API correctness tests

```bash
pxfs_api_test -h 10000   # open/read/write/pread/pwrite/lseek/mkdir/stat/...
kvfs_api_test -h 10000   # put/get/del/overwrite/binary/large-value/sync
```

## CI

Every push to master runs three jobs (Ubuntu 22.04, x86-64):

| Job | Flags | What it does |
|-----|-------|-------------|
| **CMake build** | `-DRPC=fast -DSCMPOOL=kernel` | Release build, must be warning-free |
| **ubench** | `-DRPC=net -DSCMPOOL=user` | Smoke-tests all filesystem variants; results posted to step summary |
| **Docker** | — | Builds runtime image; pushes to GHCR on `v*` tags |

## Repository layout

```
libfs/
  src/
    bcs/       Basic Communication Services (TCP/shared-mem RPC + IPC)
    scm/       SCM abstraction (persistent regions, pool allocator)
    osd/       Object Storage Device layer (locking, journaling, versioning)
    pxfs/      POSIX filesystem (server + client + mkfs)
    cfs/       Cache filesystem variant
    rxfs/      Read-only client variant
    kvfs/      Key-value store variant
    common/    Shared utilities, timing (hrtime), errno
  bench/
    ubench/    Micro-benchmarks + API unit tests
  libfs.ini    Runtime configuration
scripts/       CI, Docker, and dev helper scripts
linux/
  linux-3.9/   Patched Linux 3.9 kernel with SCM syscall support
  scmdisk/     Optional kernel block device emulating SCM latency
  scmmodel/    SCM latency modeling tools
  tools/       Build and utility tools
```

## Kernel module (optional — `SCMPOOL=kernel`)

The `linux/linux-3.9/` tree is a forward-port of Sankar's SCM patch
to Linux 3.9. It adds two custom syscalls (312/313) for NVM memory protection.
You only need this if you want to use `SCMPOOL=kernel`; the default `user` pool
works on stock Linux.

**Compilation requires a native x86-64 Linux environment** (Docker/EC2/VM — not macOS):

```bash
cd linux/linux-3.9
make mrproper
make x86_64_defconfig    # or: make menuconfig  (enable CONFIG_SCM=y)
make -j$(nproc)
sudo make modules_install install
sudo update-grub && sudo reboot
```

SCM-specific kernel config options:

```
CONFIG_SCM=y           # Enable SCM support
CONFIG_SCM_DEVICE=y    # SCM device driver
```

The patch modifies 17 files across `mm/`, `kernel/`, `arch/x86/`, and
`include/linux/` — all integrated; no manual patching needed.

## Architecture

```
Client Process                      Server Process
       ↓                                   ↓
 pxfs_api_test / kvfs_api_test      pxfs_server / kvfs_server
       ↓                                   ↓
 Client Library (pxfs/client)       Server Library (pxfs/server)
       ↓                                   ↓
 OSD Client Layer ←── RPC ──────→  OSD Server Layer
       ↓                                   ↓
 BCS (shared-mem IPC)               BCS (shared-mem IPC)
       ↓                                   ↓
            SCM Pool (persistent mmap'd file)
```

## Dependencies

- [libconfig++](http://hyperrealm.github.io/libconfig/)
- [Boost C++ Libraries](https://www.boost.org/)
- [Google Sparsehash](https://github.com/sparsehash/sparsehash)
