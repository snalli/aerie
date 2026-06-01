#!/bin/bash
# ---------------------------------------------------------------------------
# aerie-bench.sh  –  run the pxfs micro-benchmark against a running server
#
# Environment variables:
#   SERVER_HOST   server hostname / IP   (default: server)
#   PORT          server port            (default: 10000)
#   BENCH_OPS     benchmark operations   (default: +fs_create)
#   BENCH_PATH    filesystem path prefix (default: /pxfs)
#   BENCH_N       number of ops          (default: 1000)
#   BENCH_SIZE    per-file size (bytes)  (default: 4096)
# ---------------------------------------------------------------------------
set -euo pipefail

SERVER_HOST="${SERVER_HOST:-server}"
PORT="${PORT:-10000}"
BENCH_OPS="${BENCH_OPS:-+fs_create}"
BENCH_PATH="${BENCH_PATH:-/pxfs}"
BENCH_N="${BENCH_N:-1000}"
BENCH_SIZE="${BENCH_SIZE:-4096}"

exec ubench_pxfs \
    -h "${PORT}" \
    -d 0 \
    "${BENCH_OPS}" \
    -p "${BENCH_PATH}" \
    -n "${BENCH_N}" \
    -s "${BENCH_SIZE}"
