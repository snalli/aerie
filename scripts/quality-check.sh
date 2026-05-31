#!/bin/bash
# quality-check.sh - Comprehensive code quality checks
# Usage: ./scripts/quality-check.sh [--format|--analyze|--all]

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Determine what to run
TARGET="${1:---all}"

if [[ "$TARGET" != "--format" && "$TARGET" != "--analyze" && "$TARGET" != "--all" ]]; then
    echo "Usage: $0 [--format|--analyze|--all]"
    echo ""
    echo "  --format   Check code formatting with clang-format"
    echo "  --analyze  Run static analysis with cppcheck"
    echo "  --all      Run all checks (default)"
    exit 1
fi

echo ""
echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Aerie Project Code Quality Check     ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""

# Formatting check
if [[ "$TARGET" == "--format" || "$TARGET" == "--all" ]]; then
    echo -e "${BLUE}[1/2] Code Formatting (clang-format)${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    bash scripts/format-code.sh --check || true
    echo ""
fi

# Static analysis
if [[ "$TARGET" == "--analyze" || "$TARGET" == "--all" ]]; then
    echo -e "${BLUE}[2/2] Static Analysis (cppcheck)${NC}"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    bash scripts/analyze-code.sh warnings || true
    echo ""
fi

echo -e "${BLUE}╔════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Quality Check Summary                ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════╝${NC}"
echo ""
echo "Quick reference:"
echo "  • Fix formatting: ${GREEN}bash scripts/format-code.sh --fix${NC}"
echo "  • Detailed analysis: ${GREEN}bash scripts/analyze-code.sh all${NC}"
echo "  • Check critical issues: ${GREEN}bash scripts/analyze-code.sh critical${NC}"
echo ""
