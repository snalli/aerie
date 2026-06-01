# /ship — Pre-commit quality gate

Run format → build → tests locally, then commit and push only if all pass.

> **Note:** This codebase is x86-64 only. All Docker commands use
> `--platform linux/amd64` (QEMU on Apple Silicon — ~3-5x slower).
> Server-dependent tests (pxfs, cfs) are skipped locally because
> `MAP_FIXED` shared-memory addresses conflict under QEMU emulation;
> they run fine in CI on real x86 hardware.

## Stages (in order, abort on first failure)

1. **Format** — clang-format fix + verify (runs natively in Docker)
2. **Build** — cmake + make with `-DRPC=net -DSCMPOOL=user -DBUILD_BENCH=ON`
3. **Tests** — VFS baseline (no server needed, works under QEMU)
4. **Commit & push** — only if all above passed

## How to use

```
/ship
/ship "your commit message here"
```

---

## Implementation

Execute the following steps **in sequence** inside the project root
`/Users/sankethnalli/Documents/GitHub/aerie`. Stop and report clearly on failure — do NOT commit.

### Step 0 — Sanity check + image build

```bash
cd /Users/sankethnalli/Documents/GitHub/aerie
docker info > /dev/null 2>&1 || { echo "❌ Docker not running"; exit 1; }
docker build --platform linux/amd64 -f docker/Dockerfile.ci -t aerie-ci:latest . --quiet
```

### Step 1 — Format (fix then verify)

Run inside Docker so we use the exact same clang-format version as CI (22.x from LLVM).

```bash
docker run --platform linux/amd64 --rm -v "$(pwd):/workspace" aerie-ci:latest bash -c "
  cd /workspace
  bash scripts/format-code.sh --fix
  bash scripts/format-code.sh --check
"
```

If any files still fail → show them and stop.

### Step 2 — Build

Use `-DRPC=net -DSCMPOOL=user` (user-space pool, no kernel SCM) so the server
can actually run locally.

```bash
docker run --platform linux/amd64 --rm -v "$(pwd):/workspace" \
  -e LIBFS_CONFIG=/workspace/libfs/libfs.ini \
  aerie-ci:latest bash -c "
    rm -rf /workspace/libfs/build-local
    cmake -S /workspace/libfs -B /workspace/libfs/build-local \
      -DCMAKE_BUILD_TYPE=Debug -DRPC=net -DSCMPOOL=user -DBUILD_BENCH=ON
    cmake --build /workspace/libfs/build-local --parallel \$(nproc)
"
```

If build fails → show compiler errors and stop.

### Step 3 — Tests (VFS only — server tests run in CI on real x86)

```bash
docker run --platform linux/amd64 --rm -v "$(pwd):/workspace" \
  -e LIBFS_CONFIG=/workspace/libfs/libfs.ini \
  aerie-ci:latest bash -c "
    BUILD=/workspace/libfs/build-local

    echo '--- pool ---'
    \$BUILD/src/scm/pool_tool create -p /tmp/pool -s 128M

    echo '--- vfs ---'
    mkdir -p /tmp/vfsbench
    \$BUILD/bench/ubench/ubench_vfs \
      +fs_create -n 1 -p /tmp/vfsbench \
      +fs_open   -n 1 -p /tmp/vfsbench \
      +fs_read   -n 1 -p /tmp/vfsbench
"
```

If VFS test exits non-zero → show output and stop.

### Step 4 — Commit & push

Only reach this step if all above passed.

Use the message from `$ARGUMENTS` if provided, otherwise ask the user for one.

```bash
cd /Users/sankethnalli/Documents/GitHub/aerie
git add -A
git commit -m "<message>

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
git push origin master
```

## Output format

```
╔══════════════════════════════════════╗
║         /ship quality gate           ║
╠══════════════════════════════════════╣
║ Format    ✅ / ❌                    ║
║ Build     ✅ / ❌                    ║
║ Tests     ✅ VFS pass / ❌           ║
╠══════════════════════════════════════╣
║ Commit & push  ✅ pushed / ⏸ blocked ║
╚══════════════════════════════════════╝
```

Note: server tests (PXFS, CFS, OSD) are validated by the CI `bench` job on
real x86 hardware after push. Check the GitHub Actions summary for those results.
