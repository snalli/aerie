# Aerie

A usermode filesystem for Non-Volatile Memory (NVM / Storage Class Memory).

Aerie runs entirely in userspace — no kernel filesystem module required. It maps a persistent storage pool directly into process address space via `mmap` and provides POSIX filesystem semantics through a client library that communicates with a local server process over shared memory + RPC.

## Architecture

```
┌─────────────────────────────────────────┐
│  Application                            │
│  (links libfsc / libc_api)              │
└────────────────┬────────────────────────┘
                 │ IPC (shared memory + RPC)
┌────────────────▼────────────────────────┐
│  pxfs_server                            │
│  ├── OSD layer  (locking, journaling)   │
│  ├── MFS backend (directory/file inodes)│
│  └── SCM pool   (mmap'd persistent file)│
└─────────────────────────────────────────┘
```

The storage pool is a regular file memory-mapped at a fixed virtual address. No kernel module or special hardware is required; SCM write latency is emulated in userspace via `movnti` + busy-spin.

## Filesystem variants

| Name | Description |
|------|-------------|
| **pxfs** | POSIX-compliant filesystem — primary variant |
| **cfs** | Cache filesystem variant |
| **rxfs** | Read-only / replica client |
| **kvfs** | Key-value store filesystem |

## Building

### Requirements

- Linux x86-64 (code uses `rdtsc`, SSE `movnti`, and `cpu_set_t`)
- CMake ≥ 3.16
- GCC / Clang with C++11 support
- `libconfig++` (`libconfig++-dev` on Debian/Ubuntu)
- Boost headers (`libboost-dev`)
- Google Sparsehash (`libsparsehash-dev`)

### Native build

```bash
cmake -S libfs -B libfs/build -DRPC=fast
cmake --build libfs/build --parallel $(nproc)
```

**Build options:**

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `RPC` | `net`, `fast`, `fast-two` | `net` | RPC transport |
| `SCMPOOL` | `kernel`, `user` | `kernel` | SCM pool allocator |
| `BUILD_BENCH` | `ON`, `OFF` | `ON` | Build benchmark programs |
| `LIBCONFIG_ROOT` | path | — | Local libconfig build root |
| `GOOGLE_SPARSEHASH` | path | — | Sparsehash headers (if not on system path) |

### Docker (recommended)

Docker is the easiest way to get a working build — no need to install dependencies or worry about ASLR settings.

```bash
# Build and start the pxfs server
docker compose up --build

# Run the micro-benchmark against the running server
docker compose --profile bench run --rm bench
```

Override defaults via environment variables:

```bash
POOL_SIZE=1024M PORT=10000 docker compose up --build
```

> **Note:** `PersistentRegion` uses `MAP_FIXED` to remap the pool at its original virtual address on restart. ASLR must be disabled (`kernel.randomize_va_space=0`). The `docker-compose.yml` sets this automatically via `sysctls`.

## Running manually

```bash
BUILD=libfs/build

# 1. Create storage pool
$BUILD/src/scm/tool/pool/pool_tool create -p /tmp/stamnos_pool -s 512M

# 2. Format the filesystem
$BUILD/src/pxfs/pxfs_mkfs create -p /tmp/stamnos_pool -s 512M -t mfs

# 3. Start the server (background)
$BUILD/src/pxfs/pxfs_server -p 10000 -d 0 -s /tmp/stamnos_pool &

# 4. Run a benchmark
$BUILD/bench/ubench/ubench_pxfs -h 10000 -d 0 +fs_create -p /pxfs/ -n 1000 -s 4096
```

Configuration is read from `libfs/libfs.ini` (or the path in `$LIBFS_CONF`).

## Repository layout

```
libfs/                  Userspace filesystem library
  src/
    bcs/                Basic Communication Services (RPC + IPC)
    scm/                SCM abstraction (persistent regions, pool allocator)
    osd/                Object Storage Device layer (locking, journaling)
    pxfs/               POSIX filesystem (server + client + mkfs tool)
    cfs/                Cache filesystem variant
    rxfs/               Read-only client variant
    kvfs/               Key-value store variant
    common/             Shared utilities
  bench/                Micro-benchmarks
  scripts/              Docker entrypoint scripts
kernelmode/
  scmdisk/              Optional Linux kernel block device that emulates
                        SCM latency at the block layer (not required for
                        the userspace filesystem)
```

## Dependencies

- [libconfig++](http://hyperrealm.github.io/libconfig/)
- [Boost C++ Libraries](https://www.boost.org/)
- [Google Sparsehash](https://github.com/sparsehash/sparsehash)
