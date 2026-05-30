#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# run-tests.sh — build Aerie in Docker and run servers + benchmarks + API tests
#
# Usage:
#   ./run-tests.sh                 # build + run pxfs api test + ubench
#   ./run-tests.sh api             # just the pxfs/kvfs API correctness tests
#   ./run-tests.sh bench           # just the ubench micro-benchmarks
#   ./run-tests.sh all             # api + bench for every filesystem variant
#   ./run-tests.sh shell           # drop into a shell in the build container
#
# Environment:
#   POOL_SIZE   storage pool size           (default: 128M)
#   BUILD_TYPE  Debug | Release             (default: Debug)
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

docker run --rm -it \
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

# ---- dependencies (cached layer would be nicer, but image is vanilla) -------
apt-get update -qq
apt-get install -y --no-install-recommends \
  build-essential cmake libconfig++-dev libboost-dev libsparsehash-dev \
  iproute2 >/dev/null 2>&1

# ---- configure + build ------------------------------------------------------
echo "=== Building (${BUILD_TYPE}, RPC=net, SCMPOOL=user) ==="
cmake -S /aerie/libfs -B /tmp/build \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" -DRPC=net -DSCMPOOL=user -DBUILD_BENCH=ON \
  2>&1 | tail -1
cmake --build /tmp/build --parallel "$(nproc)" 2>&1 | grep -E "error:|Built target ubench_pxfs|Built target pxfs_api_test" || true
echo "=== Build done ==="

if [ "${MODE}" = "shell" ]; then exec bash; fi

POOL=/tmp/stamnos_pool
B=/tmp/build/bench/ubench

# ---- helper: start pxfs server, wait for it, run a command, stop it ---------
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

case "${MODE}" in
  api)            run_api ;;
  bench)          run_bench ;;
  all)            run_api; run_bench ;;
  default|*)      run_api; run_bench ;;
esac

echo ""
echo "=== ALL DONE ==="
INNER
