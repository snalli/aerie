#!/bin/bash
# analyze-code.sh - Run static analysis with cppcheck
# Usage: ./scripts/analyze-code.sh [OPTIONS]

# Don't exit on error - we want to report issues without failing CI
set +e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# Color codes
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Analysis level (default: all)
LEVEL="${1:-all}"

if [[ "$LEVEL" != "all" && "$LEVEL" != "warnings" && "$LEVEL" != "critical" ]]; then
    echo "Usage: $0 [all|warnings|critical]"
    echo ""
    echo "  all       Report all issues (default)"
    echo "  warnings  Report warnings and errors only"
    echo "  critical  Report critical issues only"
    exit 1
fi

echo "=========================================="
echo "cppcheck Static Analysis"
echo "=========================================="
echo "Analysis level: $LEVEL"
echo ""

# Run cppcheck with appropriate settings
CPPCHECK_ARGS=(
    "--enable=all"
    "--suppress=missingIncludeSystem"
    "--suppress=unusedFunction"
    "--suppress=unmatchedSuppression"
    "--suppress=missingInclude"
    "--std=c++11"
)

case "$LEVEL" in
    all)
        CPPCHECK_ARGS+=("--template=gcc")
        echo "Scanning: libfs/src, libfs/bench/ubench, libfs/bench/sharing"
        echo ""
        cppcheck "${CPPCHECK_ARGS[@]}" \
            libfs/src libfs/bench/ubench libfs/bench/sharing 2>&1 | \
            grep -v "^libfs/CMakeFiles" | \
            grep -v "^$" | \
            head -200
        ;;
    warnings)
        CPPCHECK_ARGS+=("--template=gcc")
        echo "Scanning for warnings and errors..."
        echo ""
        cppcheck "${CPPCHECK_ARGS[@]}" \
            libfs/src libfs/bench/ubench libfs/bench/sharing 2>&1 | \
            grep -v "^libfs/CMakeFiles" | \
            grep -v "information:" | \
            grep -v "^$" | \
            head -100
        ;;
    critical)
        echo "Scanning for critical issues..."
        echo ""
        cppcheck "${CPPCHECK_ARGS[@]}" \
            libfs/src libfs/bench/ubench libfs/bench/sharing 2>&1 | \
            grep -v "^libfs/CMakeFiles" | \
            grep -E "(nullPointer|memoryLeak|bufferOverrun|outOfBounds)" | \
            head -100
        ;;
esac

echo ""
echo "=========================================="
echo "Analysis complete"
echo "=========================================="
echo ""
echo "Common issue types:"
echo "  • nullPointer: Potential null pointer dereference"
echo "  • memoryLeak: Potential memory leak"
echo "  • bufferOverrun: Potential buffer overflow"
echo "  • outOfBounds: Array index out of bounds"
echo "  • invalidPrintfArgType: printf format string mismatch"
