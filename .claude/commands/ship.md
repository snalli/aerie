# /ship — Pre-commit quality gate

Run the full local quality pipeline inside the CI-matching Docker container.
Only if **every** stage passes will the changes be committed and pushed.

> **Note:** This codebase is x86-64 only (uses MMX intrinsics + x86 assembly).
> All Docker commands run with `--platform linux/amd64` so they work correctly
> on Apple Silicon Macs via QEMU emulation. Expect ~3-5x slower builds than CI.

## Stages (run in order, abort on first failure)

1. **Format** — `clang-format` fix + verify
2. **Build** — `cmake` + `make` inside Docker (Ubuntu 22.04 x86-64, matching CI deps)
3. **Static analysis** — `cppcheck` critical-issues check
4. **Tests** — ubench smoke suite (vfs baseline; no server required)
5. **Coverage** — `gcov` line-coverage report; warn if <50%, fail if <25%
6. **Commit & push** — only if all above passed

## How to use

```
/ship
/ship "your commit message here"
```

If a commit message is given as `$ARGUMENTS`, use it verbatim; otherwise prompt for one.

---

## Implementation

Execute the following steps **in sequence**. Stop and report the failure clearly if any step fails — do NOT commit.

Define this helper at the start so every step uses it:

```bash
DOCKER="docker run --platform linux/amd64 --rm -v $(pwd):/workspace"
IMAGE="aerie-ci:latest"
```

### Step 0 — Sanity check + image build

```bash
cd /Users/sankethnalli/Documents/GitHub/aerie

docker info > /dev/null 2>&1 || { echo "❌ Docker is not running"; exit 1; }

# Build image for linux/amd64 (x86-64) if not present or if Dockerfile.ci changed
docker build --platform linux/amd64 -f Dockerfile.ci -t aerie-ci:latest .
```

Always rebuild so Dockerfile.ci changes are picked up. Docker layer caching makes this fast when nothing changed.

### Step 1 — Format (fix then verify)

```bash
docker run --platform linux/amd64 --rm -v "$(pwd):/workspace" aerie-ci:latest bash -c "
  cd /workspace
  # Fix formatting in-place
  find libfs -name '*.cc' -o -name '*.h' -o -name '*.c' | \
    xargs clang-format -i --style=file
  # Verify nothing is still dirty
  find libfs -name '*.cc' -o -name '*.h' -o -name '*.c' | \
    xargs clang-format --style=file --dry-run --Werror
"
```

If this fails → report which files are still misformatted and stop.

### Step 2 — Build

```bash
docker run --platform linux/amd64 --rm -v "$(pwd):/workspace" aerie-ci:latest bash -c "
  rm -rf /workspace/libfs/build
  cmake -S /workspace/libfs -B /workspace/libfs/build \
    -DCMAKE_BUILD_TYPE=Release -DRPC=fast -DSCMPOOL=kernel -DBUILD_BENCH=ON
  cmake --build /workspace/libfs/build --parallel \$(nproc)
"
```

If this fails → show the compiler errors and stop.

### Step 3 — Static analysis (cppcheck)

```bash
docker run --platform linux/amd64 --rm -v "$(pwd):/workspace" aerie-ci:latest bash -c "
  cppcheck --enable=warning,error \
    --error-exitcode=1 \
    --suppress=missingIncludeSystem \
    --inline-suppr \
    -I /workspace/libfs/src \
    /workspace/libfs/src 2>&1
"
```

If cppcheck reports errors → show them and stop.

### Step 4 — Tests (ubench smoke)

```bash
docker run --platform linux/amd64 --rm -v "$(pwd):/workspace" aerie-ci:latest bash -c "
  BUILD=/workspace/libfs/build

  # Create SCM pool
  \$BUILD/src/scm/pool_tool create -p /tmp/stamnos_pool -s 128M

  # VFS baseline (no server needed)
  mkdir -p /tmp/vfsbench
  \$BUILD/bench/ubench/ubench_vfs \
    +fs_create -n 1 -p /tmp/vfsbench \
    +fs_open   -n 1 -p /tmp/vfsbench \
    +fs_read   -n 1 -p /tmp/vfsbench
"
```

If any test exits non-zero → show output and stop.

### Step 5 — Coverage (best-effort, warn only)

```bash
docker run --platform linux/amd64 --rm -v "$(pwd):/workspace" aerie-ci:latest bash -c "
  # Rebuild with coverage flags
  cmake -S /workspace/libfs -B /workspace/libfs/build-cov \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS='--coverage' \
    -DCMAKE_EXE_LINKER_FLAGS='--coverage' \
    -DRPC=fast -DSCMPOOL=kernel -DBUILD_BENCH=ON
  cmake --build /workspace/libfs/build-cov --parallel \$(nproc)

  # Run vfs smoke test to generate .gcda files
  mkdir -p /tmp/vfsbench-cov
  /workspace/libfs/build-cov/bench/ubench/ubench_vfs \
    +fs_create -n 1 -p /tmp/vfsbench-cov \
    +fs_open   -n 1 -p /tmp/vfsbench-cov \
    +fs_read   -n 1 -p /tmp/vfsbench-cov || true

  # Report with gcovr
  gcovr --root /workspace/libfs \
    --object-directory /workspace/libfs/build-cov \
    --filter '/workspace/libfs/src/' \
    --print-summary 2>/dev/null | grep -E 'lines|branches' || echo 'Coverage: n/a'
" 2>&1 || echo "⚠️  Coverage step skipped (non-fatal)"
```

Parse the coverage percentage:
- **≥ 50%** → ✅ pass
- **25–49%** → ⚠️  warn but continue
- **< 25%** → ❌ fail, stop

### Step 6 — Commit & push

Only reach this step if all above passed.

```bash
cd /Users/sankethnalli/Documents/GitHub/aerie
git add -A
git commit -m "<commit message from $ARGUMENTS or prompt user>"
git push origin master
```

Append to the commit message:
```
Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

## Output format

Print a summary table at the end:

```
╔══════════════════════════════════════╗
║         /ship quality gate           ║
╠══════════════════════════════════════╣
║ Format         ✅ / ❌               ║
║ Build          ✅ / ❌               ║
║ Static checks  ✅ / ❌               ║
║ Tests          ✅ / ❌               ║
║ Coverage       ✅ XX% / ⚠️ XX% / ❌  ║
╠══════════════════════════════════════╣
║ Commit & push  ✅ pushed / ⏸ blocked ║
╚══════════════════════════════════════╝
```
