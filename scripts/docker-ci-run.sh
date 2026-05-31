#!/bin/bash
# docker-ci-run.sh - Run CI checks in Docker environment matching GitHub Actions
# Usage: bash scripts/docker-ci-run.sh [command]
#
# Examples:
#   bash scripts/docker-ci-run.sh bash           # Interactive shell
#   bash scripts/docker-ci-run.sh format-code    # Run formatter check
#   bash scripts/docker-ci-run.sh build          # Run full build

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

COMMAND="${1:-bash}"

echo "═══════════════════════════════════════════════════════════════════════════════"
echo "Aerie CI Environment (Docker)"
echo "═══════════════════════════════════════════════════════════════════════════════"
echo ""

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "❌ Docker is not installed. Please install Docker Desktop."
    echo "   https://www.docker.com/products/docker-desktop"
    exit 1
fi

# Build image if it doesn't exist
if ! docker image inspect aerie-ci:latest &> /dev/null; then
    echo "📦 Building Docker image (first run only)..."
    echo ""
    docker build -f Dockerfile.ci -t aerie-ci:latest .
    echo ""
fi

# Prepare command
case "$COMMAND" in
    bash)
        echo "🚀 Starting interactive shell in CI environment"
        echo ""
        echo "Available commands inside container:"
        echo "  bash scripts/format-code.sh --check    # Check formatting"
        echo "  bash scripts/format-code.sh --fix      # Fix formatting"
        echo "  bash scripts/analyze-code.sh critical  # Run static analysis"
        echo "  bash scripts/quality-check.sh --all    # Run all checks"
        echo "  cd libfs && cmake . && make            # Build project"
        echo ""
        docker run --rm -it -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            aerie-ci:latest bash
        ;;
    format-code)
        echo "🔍 Checking code formatting..."
        echo ""
        docker run --rm -it -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            aerie-ci:latest bash scripts/format-code.sh --check
        ;;
    format-fix)
        echo "✏️  Fixing code formatting..."
        echo ""
        docker run --rm -it -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            aerie-ci:latest bash scripts/format-code.sh --fix
        ;;
    analyze)
        echo "🔬 Running static analysis..."
        echo ""
        docker run --rm -it -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            aerie-ci:latest bash scripts/analyze-code.sh critical
        ;;
    quality)
        echo "📊 Running all quality checks..."
        echo ""
        docker run --rm -it -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            aerie-ci:latest bash scripts/quality-check.sh --all
        ;;
    build)
        echo "🏗️  Building project in CI environment..."
        echo ""
        docker run --rm -it -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            aerie-ci:latest bash -c "cd libfs && cmake . && make -j\$(nproc)"
        ;;
    ci-full)
        echo "🚀 Running full CI pipeline..."
        echo ""
        docker run --rm -it -v "$PROJECT_ROOT:/workspace" \
            -w /workspace \
            aerie-ci:latest bash -c "
                echo '=== Formatting Check ===' && \
                bash scripts/format-code.sh --check && \
                echo '' && \
                echo '=== Static Analysis ===' && \
                bash scripts/analyze-code.sh critical && \
                echo '' && \
                echo '=== Build Test ===' && \
                cd libfs && cmake . && make -j\$(nproc) && \
                echo '' && \
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
