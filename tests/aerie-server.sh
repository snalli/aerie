#!/bin/bash
# ---------------------------------------------------------------------------
# aerie-server.sh  –  initialise storage pool (if needed) and start pxfs_server
#
# Environment variables:
#   POOL_PATH   path to the pool file           (default: /data/stamnos_pool)
#   POOL_SIZE   pool size passed to pool_tool   (default: 512M)
#   FS_TYPE     filesystem type for mkfs        (default: mfs)
#   PORT        server TCP port                 (default: 10000)
#   DEBUG       debug level 0-5                 (default: 0)
# ---------------------------------------------------------------------------
set -euo pipefail

POOL_PATH="${POOL_PATH:-/data/stamnos_pool}"
POOL_SIZE="${POOL_SIZE:-512M}"
FS_TYPE="${FS_TYPE:-mfs}"
PORT="${PORT:-10000}"
DEBUG="${DEBUG:-0}"

# Disable ASLR — PersistentRegion uses MAP_FIXED and stores the mapped base
# address in the pool header; remapping fails if the address shifts.
if [ -w /proc/sys/kernel/randomize_va_space ]; then
    echo 0 > /proc/sys/kernel/randomize_va_space
else
    echo "WARNING: cannot disable ASLR (no write access to /proc/sys/kernel/randomize_va_space)" >&2
    echo "         Run the container with --sysctl kernel.randomize_va_space=0" >&2
fi

# Clean up any leftover shared buffers from a previous run
rm -f /tmp/shbuf_*

# Create pool + filesystem on first run
if [ ! -f "${POOL_PATH}" ]; then
    echo "==> Creating storage pool at ${POOL_PATH} (${POOL_SIZE})"
    pool_tool create -p "${POOL_PATH}" -s "${POOL_SIZE}"

    echo "==> Formatting filesystem (type: ${FS_TYPE})"
    pxfs_mkfs create -p "${POOL_PATH}" -s "${POOL_SIZE}" -t "${FS_TYPE}"
fi

echo "==> Starting pxfs_server on port ${PORT}"
exec pxfs_server -p "${PORT}" -d "${DEBUG}" -s "${POOL_PATH}"
