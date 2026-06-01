#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# run-tests.sh — build Aerie in Docker and run servers + benchmarks + API tests
#
# Usage:
#   ./run-tests.sh                 # build + run pxfs api test + ubench
#   ./run-tests.sh api             # just the pxfs API correctness tests
#   ./run-tests.sh bench           # just the ubench micro-benchmarks
#   ./run-tests.sh all             # api + bench
#   ./run-tests.sh leak            # run pxfs API tests under valgrind leak-check
#   ./run-tests.sh kvfs            # run kvfs API tests
#   ./run-tests.sh kvfs-leak       # run kvfs API tests under valgrind leak-check
#   ./run-tests.sh shell           # drop into a shell in the build container
#
# Environment:
#   POOL_SIZE   storage pool size   (default: 128M)
#   BUILD_TYPE  Debug | Release     (default: Debug)
#
# Requirements: Docker Desktop running (linux/amd64; Rosetta on Apple Silicon)
# ---------------------------------------------------------------------------
set -euo pipefail

REPO="$(cd "$(dirname "$0")" && pwd)"
MODE="${1:-default}"
POOL_SIZE="${POOL_SIZE:-128M}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"

# Stop any leftover containers from previous runs (they hold the TCP port)
docker ps -q --filter "ancestor=ubuntu:22.04" | xargs -r docker stop >/dev/null 2>&1 || true

docker run --rm -i \
  --platform linux/amd64 \
  -v "${REPO}:/aerie:ro" \
  -v "aerie-build-cache:/tmp/build" \
  -e LIBFS_CONFIG=/aerie/libfs/libfs.ini \
  -e MODE="${MODE}" \
  -e POOL_SIZE="${POOL_SIZE}" \
  -e BUILD_TYPE="${BUILD_TYPE}" \
  ubuntu:22.04 \
  bash <<'INNER'
set -e
export DEBIAN_FRONTEND=noninteractive

# ---- dependencies -----------------------------------------------------------
apt-get update -qq
PKGS="build-essential cmake libconfig++-dev libboost-dev libsparsehash-dev iproute2"
[ "${MODE}" = "leak" ] || [ "${MODE}" = "kvfs-leak" ] && PKGS="${PKGS} valgrind"
[ "${MODE}" = "coverage" ] && PKGS="${PKGS} lcov"
apt-get install -y --no-install-recommends ${PKGS} >/dev/null 2>&1

# ---- configure + build ------------------------------------------------------
echo "=== Building (${BUILD_TYPE}, RPC=net, SCMPOOL=user) ==="
COV_FLAG=""
[ "${MODE}" = "coverage" ] && COV_FLAG="-DCOVERAGE=ON"
cmake -S /aerie/libfs -B /tmp/build \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DRPC=net -DSCMPOOL=user -DBUILD_BENCH=ON ${COV_FLAG} \
  2>&1 | tail -1
cmake --build /tmp/build --parallel "$(nproc)" 2>&1 \
  | grep -E "error:|Built target ubench_pxfs|Built target pxfs_api_test" || true
echo "=== Build done ==="

[ "${MODE}" = "shell" ] && exec bash

POOL=/tmp/stamnos_pool
B=/tmp/build/tests/benchmark/ubench

start_pxfs() {
  /tmp/build/src/scm/pool_tool  create -p "${POOL}" -s "${POOL_SIZE}" 2>&1 | tail -1
  /tmp/build/src/pxfs/pxfs_mkfs create -p "${POOL}" -s "${POOL_SIZE}" -t mfs 2>&1 | tail -1
  rm -f /tmp/shbuf_*
  /tmp/build/src/pxfs/pxfs_server -p 10000 -d 0 -s "${POOL}" &
  PXFS_PID=$!
  for i in $(seq 1 30); do ss -ltn 2>/dev/null | grep -q ":10000" && break; sleep 1; done
  ss -ltn | grep -q ":10000" || { echo "pxfs_server failed to start"; exit 1; }
  sleep 3
}
stop_pxfs() { kill "${PXFS_PID}" 2>/dev/null || true; wait "${PXFS_PID}" 2>/dev/null || true; rm -f /tmp/shbuf_*; }

start_kvfs() {
  KVFS_POOL=/tmp/stamnos_pool
  rm -f "${KVFS_POOL}"
  /tmp/build/src/scm/pool_tool  create -p "${KVFS_POOL}" -s "${POOL_SIZE}" 2>&1 | tail -1
  /tmp/build/src/kvfs/kvfs_mkfs create -p "${KVFS_POOL}" -s "${POOL_SIZE}" 2>&1 | tail -1
  rm -f /tmp/shbuf_*
  /tmp/build/src/kvfs/kvfs_server -p 10001 -d 0 -s "${KVFS_POOL}" &
  KVFS_PID=$!
  for i in $(seq 1 30); do ss -ltn 2>/dev/null | grep -q ":10001" && break; sleep 1; done
  ss -ltn | grep -q ":10001" || { echo "kvfs_server failed to start"; exit 1; }
  sleep 3
}

stop_kvfs() { kill "${KVFS_PID}" 2>/dev/null || true; wait "${KVFS_PID}" 2>/dev/null || true; rm -f /tmp/shbuf_*; }

run_api() {
  start_pxfs
  echo ""
  echo "=== pxfs_api_test ==="
  "${B}/pxfs_api_test" -h 10000
  stop_pxfs
}

run_bench() {
  start_pxfs
  echo ""
  echo "=== ubench_pxfs ==="
  "${B}/ubench_pxfs" -h 10000 \
    +fs_create   -n 100 -p /pxfs \
    +fs_open     -n 100 -p /pxfs \
    +fs_read     -n 100 -p /pxfs \
    +fs_seqwrite -n 1   -p /pxfs \
    +fs_seqread  -n 1   -p /pxfs
  stop_pxfs
}

run_leak() {
  start_pxfs
  echo ""
  echo "=== valgrind leak-check: pxfs_api_test ==="
  valgrind --leak-check=full --show-leak-kinds=definite,indirect \
           --errors-for-leak-kinds=definite --error-exitcode=99 \
           "${B}/pxfs_api_test" -h 10000
  echo "valgrind exit: $?"
  stop_pxfs
}

run_kvfs() {
  start_kvfs
  echo ""
  echo "=== kvfs_api_test ==="
  "${B}/kvfs_api_test" -h 10001
  stop_kvfs
}

run_kvfs_leak() {
  start_kvfs
  echo ""
  echo "=== valgrind leak-check: kvfs_api_test ==="
  valgrind --leak-check=full --show-leak-kinds=definite,indirect \
           --errors-for-leak-kinds=definite --error-exitcode=99 \
           "${B}/kvfs_api_test" -h 10001
  echo "valgrind exit: $?"
  stop_kvfs
}

run_coverage() {
  # /tmp/build is a read-write volume; gcov .gcda files land next to objects
  lcov --zerocounters --directory /tmp/build >/dev/null 2>&1 || true
  start_pxfs
  echo ""
  echo "=== exercising code for coverage ==="
  "${B}/pxfs_api_test" -h 10000 | tail -2
  "${B}/ubench_pxfs" -h 10000 \
    +fs_create -n 50 -p /pxfs +fs_open -n 50 -p /pxfs +fs_read -n 50 -p /pxfs \
    +fs_seqwrite -n 1 -p /pxfs +fs_seqread -n 1 -p /pxfs >/dev/null 2>&1 || true
  stop_pxfs
  echo ""
  echo "=== coverage summary (libfs/src) ==="
  lcov --capture --directory /tmp/build --output-file /tmp/cov.info \
       --rc lcov_branch_coverage=0 >/dev/null 2>&1
  # keep only the project sources, drop system headers / boost / sparsehash
  lcov --extract /tmp/cov.info "/aerie/libfs/src/*" \
       --output-file /tmp/cov.src.info >/dev/null 2>&1
  lcov --list /tmp/cov.src.info 2>/dev/null | tail -40
  lcov --summary /tmp/cov.src.info 2>&1 | grep -E "lines|functions" || true
}

case "${MODE}" in
  api)        run_api ;;
  bench)      run_bench ;;
  leak)       run_leak ;;
  kvfs)       run_kvfs ;;
  kvfs-leak)  run_kvfs_leak ;;
  coverage)   run_coverage ;;
  all)        run_api; run_bench ;;
  default|*)  run_api; run_bench ;;
esac

echo ""
echo "=== ALL DONE ==="
INNER
