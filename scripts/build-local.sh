#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# build-local.sh  –  reproduce the CI cmake build inside a Docker container
#
# Usage:
#   ./build-local.sh              # build, print warnings/errors to terminal
#   ./build-local.sh 2>warn.txt   # capture output to a file
#
# Requirements: Docker Desktop running (linux/amd64 via Rosetta on Apple Silicon)
# ---------------------------------------------------------------------------
set -euo pipefail

REPO="$(cd "$(dirname "$0")" && pwd)"

docker run --rm \
  --platform linux/amd64 \
  -v "${REPO}:/aerie:ro" \
  -v "aerie-build-cache:/tmp/build" \
  ubuntu:22.04 \
  bash -c '
    set -e
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -qq
    apt-get install -y --no-install-recommends \
      build-essential cmake libconfig++-dev libboost-dev libsparsehash-dev \
      >/dev/null 2>&1
    echo "=== Configuring ==="
    cmake -S /aerie/libfs -B /tmp/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DRPC=fast \
      -DSCMPOOL=kernel \
      -DBUILD_BENCH=ON \
      2>&1
    echo "=== Building ==="
    cmake --build /tmp/build --parallel "$(nproc)" 2>&1
    echo "=== Done ==="
  '
