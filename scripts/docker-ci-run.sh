#!/bin/bash
# docker-ci-run.sh - Run CI checks in Docker environment matching GitHub Actions
# Usage: bash scripts/docker-ci-run.sh [command]
#
# NOTE: This codebase is x86-64 only (MMX intrinsics, x86 assembly).
# All containers run with --platform linux/amd64 so this works on Apple Silicon
# via QEMU emulation. Builds are ~3-5x slower than native CI.
#
# Examples:
#   bash scripts/docker-ci-run.sh bash           # Interactive shell
#   bash scripts/docker-ci-run.sh format-code    # Run formatter check
#   bash scripts/docker-ci-run.sh build          # Run full build

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

COMMAND="${1:-bash}"
PLATFORM="--platform linux/amd64"
IMAGE="aerie-ci:latest"

echo "═══════════════════════════════════════════════════════════════════════════════"
echo "Aerie CI Environment (Docker, linux/amd64)"
echo "═══════════════════════════════════════════════════════════════════════════════"
echo ""

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "❌ Docker is not installed. Please install Docker Desktop."
    echo "   https://www.docker.com/products/docker-desktop"
    exit 1
fi

# Always build (layer cache makes this fast when nothing changed)
echo "📦 Building Docker image for linux/amd64..."
docker build $PLATFORM -f Dockerfile.ci -t "$IMAGE" . --quiet
echo ""

# Prepare command
case "$COMMAND" in
    bash)
        echo "🚀 Starting interactive shell in CI environment (linux/amd64)"
        echo ""
        echo "Available commands inside container:"
        echo "  bash scripts/format-code.sh --check    # Check formatting"
        echo "  bash scripts/format-code.sh --fix      # Fix formatting"
        echo "  bash scripts/analyze-code.sh critical  # Run static analysis"
        echo "  bash scripts/quality-check.sh --all    # Run all checks"
        echo "  cmake -S libfs -B libfs/build -DRPC=fast -DSCMPOOL=kernel -DBUILD_BENCH=ON && cmake --build libfs/build"
        echo ""
        docker run $PLATFORM --rm -it -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            "$IMAGE" bash
        ;;
    format-code)
        echo "🔍 Checking code formatting..."
        echo ""
        docker run $PLATFORM --rm -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            "$IMAGE" bash scripts/format-code.sh --check
        ;;
    format-fix)
        echo "✏️  Fixing code formatting..."
        echo ""
        docker run $PLATFORM --rm -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            "$IMAGE" bash scripts/format-code.sh --fix
        ;;
    analyze)
        echo "🔬 Running static analysis..."
        echo ""
        docker run $PLATFORM --rm -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            "$IMAGE" bash scripts/analyze-code.sh critical
        ;;
    quality)
        echo "📊 Running all quality checks..."
        echo ""
        docker run $PLATFORM --rm -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            "$IMAGE" bash scripts/quality-check.sh --all
        ;;
    build)
        echo "🏗️  Building project in CI environment (linux/amd64)..."
        echo ""
        docker run $PLATFORM --rm -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            "$IMAGE" bash -c "
                rm -rf /workspace/libfs/build &&
                cmake -S /workspace/libfs -B /workspace/libfs/build \
                  -DCMAKE_BUILD_TYPE=Release -DRPC=fast -DSCMPOOL=kernel -DBUILD_BENCH=ON &&
                cmake --build /workspace/libfs/build --parallel \$(nproc)
            "
        ;;
    ci-full)
        echo "🚀 Running full CI pipeline (linux/amd64)..."
        echo ""
        docker run $PLATFORM --rm -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            "$IMAGE" bash -c "
                echo '=== Step 1: Formatting Check ===' &&
                bash scripts/format-code.sh --check &&
                echo '' &&
                echo '=== Step 2: Static Analysis ===' &&
                bash scripts/analyze-code.sh critical &&
                echo '' &&
                echo '=== Step 3: Build ===' &&
                rm -rf /workspace/libfs/build &&
                cmake -S /workspace/libfs -B /workspace/libfs/build \
                  -DCMAKE_BUILD_TYPE=Release -DRPC=fast -DSCMPOOL=kernel -DBUILD_BENCH=ON &&
                cmake --build /workspace/libfs/build --parallel \$(nproc) &&
                echo '' &&
                echo '=== Step 4: Tests ===' &&
                /workspace/libfs/build/src/scm/pool_tool create -p /tmp/stamnos_pool -s 128M &&
                mkdir -p /tmp/vfsbench &&
                /workspace/libfs/build/bench/ubench/ubench_vfs \
                  +fs_create -n 1 -p /tmp/vfsbench \
                  +fs_open   -n 1 -p /tmp/vfsbench \
                  +fs_read   -n 1 -p /tmp/vfsbench &&
                echo '' &&
                echo '✅ All CI checks passed!'
            "
        ;;
    *)
        echo "❌ Unknown command: $COMMAND"
        echo ""
        echo "Available commands:"
        echo "  bash              Interactive shell"
        echo "  format-code       Check formatting"
        echo "  format-fix        Fix formatting"
        echo "  analyze           Run static analysis"
        echo "  quality           Run all quality checks"
        echo "  build             Build project"
        echo "  ci-full           Run complete CI pipeline"
        exit 1
        ;;
esac
